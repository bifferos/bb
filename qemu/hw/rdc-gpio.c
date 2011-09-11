/*
 * Emulation for RDC-based GPIO via PCI configuration registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "hw.h"
#include "pc.h"
#include "pci.h"
#include "rdc-gpio.h"

#define DRIVER_NAME "RDC-GPIO: "
#define PANEL_NAME "GPIO panel: "
#define PANEL_PORT 0xb1ff


// The GPIO pins we're emulating.  The others aren't connected to anything
// on the Bifferboard.
static unsigned char g_PinMap[] = { 16,15,11,13,9,12 };


#define PANEL_PINS sizeof(g_PinMap)


typedef struct panel_connection {
	int socket;    /* socket we'll connect to the panel with */
	fd_set fds;    /* list of descriptors (only the above socket */
	char last[PANEL_PINS]; /* we don't want to send updates to the panel
	                       unless something changed */
} panel_connection_t;


// Only deals with the first 'bank'.
typedef struct PCIGPIOState {
    PCIDevice dev;
    uint32_t data;      // last value the 32-bit data port took.
    uint32_t control;   // last value the 32-bit control port took.
    panel_connection_t panel;   // connection to GPIO panel
} PCIGPIOState;


/*
 * Hub connections
 *
 */

static int panel_open(panel_connection_t* h)
{
	struct sockaddr_in remote;

	if ((h->socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror(PANEL_NAME "socket");
		return -1;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(PANEL_PORT);
	remote.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(h->socket, (struct sockaddr *)&remote, sizeof(remote)) == -1) {
		perror(PANEL_NAME "connection");
		close(h->socket);
		h->socket = -1;
		return -1;
	}
	
	FD_ZERO(&h->fds);

	/* Set our connected socket */
	FD_SET(h->socket, &h->fds);	
	
	printf(PANEL_NAME "Connected OK\n");
	return 0;
}


static void panel_command(panel_connection_t* h, const char* command)
{
	if (send(h->socket, command, strlen(command), 0) == -1) {
		perror(PANEL_NAME "send");
		exit(1);
	}
}


/* Wait for values to be read back from panel */
static int panel_getpins(panel_connection_t* h, char* status, size_t slen)
{
	char str[100];
	fd_set rfds, efds;
	int t;
	// Wait for a response from peripheral, assume it arrives all together
	// (likely since it's only a few bytes long)

	rfds = h->fds;
	efds = h->fds;

	if (select(h->socket + 1, &rfds, NULL, &efds, NULL) == -1) {
		//perror(PANEL_NAME "select");
		return 0;  // syscall?
	}

	strcpy(status,"");

	if (FD_ISSET(h->socket, &rfds)) {
		/* receive more data */
		if ((t = recv(h->socket, str, sizeof(str)-1, 0)) > 0) {
			str[t] = '\0';
			if (strncmp(str,"R ",2)==0) {
				strncpy(status, &str[2],slen-1);
				/* ensure termination */
				status[slen-1] = '\0';
			} else {
				printf(PANEL_NAME "Invalid data received\n");
			}
		} else {
			if (t < 0) 
				perror("recv");
			else 
				printf(PANEL_NAME "closed connection\n");
			close(h->socket);
			h->socket = -1;  /* act like we never connected */
		}
	}

	if (FD_ISSET(h->socket, &efds)) {
		/* error on this socket */
		printf(PANEL_NAME "closed connection\n");
		close(h->socket);
		h->socket = -1;  /* act like we never connected */
	}
	return strlen(status);
}


/* Send a read request */
static int panel_read(panel_connection_t* h, char* status, size_t slen)
{
	int len=0;
	panel_command(h, "R\r\n");
	while (!len)
		len = panel_getpins(h, status, slen);
	return len;
}


/* Set a pin to a specified value */
static void panel_write(panel_connection_t* h, int pin, char val)
{
	char command[100];
	sprintf(command, "W %d %c\r\n", pin, val);
	panel_command(h, command);
}


//static void panel_close(panel_connection_t* h)
//{
//	close(h->socket);
//}


/* Bifferboard specific.  Update the pins in response to reg write */
static void bb_pin_write(panel_connection_t* h, uint32_t reg)
{
  int i;
  char tmp;
  
  for (i=0;i<PANEL_PINS;i++) {
    tmp = (reg & (1<<g_PinMap[i])) ? 'P':'0';
    if (tmp != h->last[i]) {
      /* send it and update the previous value */
      panel_write(h, i, tmp);
      h->last[i] = tmp;
    }
  }
}


/* we don't deal with pin contention yet, maybe best to let peripherals signal the
 * error 
 */
static uint32_t bb_pin_read(panel_connection_t* h)
{
  int i;
  char tmp[100];
  uint32_t out = 0xffffffff;   /* assume all values float high, except connected pins */
  
  if (panel_read(h, tmp, sizeof(tmp))) {
    for (i=0;i<PANEL_PINS;i++) {
      if (tmp[i]=='0') out &= ~(1<<g_PinMap[i]);
    }
  }
  return out;
}


static uint32_t pci_read_config(PCIDevice *d, uint32_t address, int len)
{
    PCIGPIOState *pms = container_of(d, PCIGPIOState, dev);
    uint32_t out = 0xffffffff;
    if (len!=4) return 0;

    /* Return last values written */
    if (address == 0x4c)  /* read the data register */
    {
      /* return something predictable (but obviously wrong) if we're not connected */
      if (pms->panel.socket == -1) 
      {
        out = 0;
      }
      else
      {
        /* read all ports.  Should really mask out ports which aren't configured GPIO */
        out = bb_pin_read(&pms->panel);
      }
    }
    if (address == 0x48)  /* read the control register */
    {
      /* just return the last value written */
      out = pms->control;
    }
    return out;
}


static void pci_write_config(PCIDevice *d, uint32_t address, uint32_t val, int len)
{
    PCIGPIOState *pms = container_of(d, PCIGPIOState, dev);
    if (len!=4) return;  /* assume long access only for the moment */

    if (address == 0x4c)
    {
      if (pms->panel.socket == -1)
      {
        /* do nothing if we're not connected */
      }
      else
      {
        /* Update the panel with new values as needed */
        bb_pin_write(&pms->panel, val);
      }

      /* Last value to be written */
      pms->data = val;
      return;
    }
    if (address == 0x48)
    {
      /* Just store what was written */
      pms->control = val;
      return;
    }
}


void rdc_gpio_pci_init(PCIBus *bus)
{
  int i;
  int err;
  PCIGPIOState *s = NULL;   // = DO_UPCAST(PCIGPIOState, dev, dev);
  /*struct PCIDevice* dev; */

  /* setup PCI configuration registers */
  s = (PCIGPIOState *)pci_register_device(bus, "RDC GPIO", sizeof(PCIGPIOState),
                                                 0x38, pci_read_config, pci_write_config);
                     
  /* Get access to the GPIO panel, program will quit on fail */
  err = panel_open(&s->panel);
  if (err)
  {
     printf("Couldn't connect to a GPIO panel, GPIO will be disabled.  Please run 'panel.py' *before* running Qemu to eliminate this annoying message.\n");
     return;
  }

  /* set all connected pins as weak pull-up (X is default) */
  for (i=0;i<PANEL_PINS;i++) {
    panel_write(&s->panel, i, 'P');
    /* remember the state we set then only send changes */
    s->panel.last[i] = 'P';
  }
  return;
}



/*
static PCIDeviceInfo rdc_gpio_pci_info = {
    .qdev.name = "rdc-gpio",
    .qdev.size = sizeof(PCIGPIOState),
    .init      = rdc_gpio_pci_init,
    .config_read = pci_read_config,
    .config_write = pci_write_config,
    .vendor_id = PCI_VENDOR_ID_RDC,
    .device_id = 0x6030,
    .revision  = 0x10,
    .class_id  = PCI_CLASS_BRIDGE_ISA,
};


static void rdc_pci_register_devices(void)
{
    printf("Registering\n");
    pci_qdev_register(&rdc_gpio_pci_info);
}


device_init(rdc_pci_register_devices)
*/
