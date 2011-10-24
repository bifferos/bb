//
// Emulation of EN29LV640 flash chip.
// Note there is little point in catering for more than one of these in a system, so
// everything done with statics.  Very unprofessional.. so sue me :).
//
// (c) Bifferos.com 2010 (sales@bifferos.com).


#include "hw.h"
#include "md5.h"
#include "sysemu.h"
#include "en29lv640.h"


#define FLASH_SIZE 0x800000
#define FLASH_BASE 0xff800000
#define FLASH_DEV_LEN (FLASH_SIZE - 0x10000)


#define ADDR_TO_SECLEN(addr) ((addr<0x10000) ? 0x2000 : 0x10000)
#define ADDR_TO_SECBASE(addr) ((addr<0x10000) ? (addr & 0xffffe000) : (addr & 0xffff0000))


// the state of the flash data. The top 64k is never written to.
static uint8_t g_backing[FLASH_SIZE];


static void flash_load_image(const char* fw_name)
{
    int fd, len;
    int available;
    
    // setup a sensible config block
    //write_config_block(&g_backing[0x4000]);
        
    
    fd = open(fw_name, O_RDONLY | O_BINARY);
    if (fd < 0)
    {
	printf("Unable to load flash backing file, setting to 0xff\n");
	memset(&g_backing[0], 0xff, sizeof(g_backing));
        return;
    }
    
    len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    available = sizeof(g_backing);  
    if (read(fd, &g_backing[0], available) != len) {
        if (len>available)
	{
	  printf("Truncated over-sized backing file\n");
	}
        if (len<available)
	{
	  printf("Padding backing file for flash with %d bytes\n", available - len);
	  memset(&g_backing[0+len], 0xff, available-len);
	}
    }
    close(fd);
}


void en29lv640_save_image(void)
{
    int fd = open(firmware_name, O_RDWR | O_BINARY);    
    int towrite = FLASH_DEV_LEN;
    int written = write(fd, &g_backing[0], towrite);
    if (towrite == written)
    {
      printf("Writing out flash to backing file %s\n", firmware_name);
    }
    else
    {
      printf("Error writing out flash to backing file %s (only %d bytes written)\n", firmware_name, written);
    }
    close(fd);
}


// All possible states the flash can be in.
// writes are instant, so we don't bother with data toggle state.
enum FLASH_STATE
{
  CYCLE_READ,
  CYCLE_1,
  CYCLE_2,
  CYCLE_3,
  CYCLE_4,
  CYCLE_AUTOSELECT,
  CYCLE_PROGRAM,
  CYCLE_ERASE,
  CYCLE_CFI
};


// diagnostic helper function
#if 0
static const char* state2string(enum FLASH_STATE state)
{
  switch (state)
  {
    case CYCLE_READ : return "CYCLE_READ";
    case CYCLE_1 : return "CYCLE_1";
    case CYCLE_2 : return "CYCLE_2";
    case CYCLE_3 : return "CYCLE_3";
    case CYCLE_4 : return "CYCLE_4";
    case CYCLE_AUTOSELECT : return "CYCLE_AUTOSELECT";
    case CYCLE_PROGRAM : return "CYCLE_PROGRAM";
    case CYCLE_ERASE : return "CYCLE_ERASE";
    case CYCLE_CFI : return "CYCLE_CFI";
    default:
      return "<UNRECOGNISED>";
  }
}
#endif


static void EraseSector(uint32_t addr)
{
  uint32_t len = ADDR_TO_SECLEN(addr);
  uint32_t base = ADDR_TO_SECBASE(addr);  
  unsigned char* tmp = g_backing;
  tmp += base;
  
  memset((void*)tmp, 0xff, len); 

//  printf("Erasing sector 0x%x\n", addr);
}

static void ChipErase(void)
{
  printf("Chip erase is unsupported, and will trash your bootloader anyhow!\n");
}


// where we are now.
static enum FLASH_STATE g_state;



// byte-width write
static void write_w1(uint32_t addr, uint32_t val)
{
  switch (g_state)
  {
    case CYCLE_READ:
      if ((addr==0xaaa) && (val==0xaa)) {
        g_state = CYCLE_1;
      } else if ((addr==0x55) && (val==0x98)) {
        g_state = CYCLE_CFI;
      }
      break;
    case CYCLE_1:
      if ((addr==0x555) && (val==0x55)) {
        g_state = CYCLE_2;
      }
      break;    
    case CYCLE_2:
      if (addr==0xaaa)
      {
        switch (val)
        {
          case 0x90:
            g_state = CYCLE_AUTOSELECT;
            break;
          case 0xa0:
            g_state = CYCLE_PROGRAM;
            break;
          case 0x80:
            g_state = CYCLE_3;
            break;
        }
      }
      break;
    case CYCLE_3:
      if ((addr==0xaaa) && (val==0xaa)) {
        g_state = CYCLE_4;
      }
      break;    
    case CYCLE_4:
      if ((addr==0x555) && (val==0x55)) {
        g_state = CYCLE_ERASE;
      }
      break;    
    case CYCLE_ERASE:
      if (val==0x30) {
	EraseSector(addr);
      } else if (val==0x10) {
	ChipErase();
      }
      break;    
    case CYCLE_AUTOSELECT:
      break;
    case CYCLE_PROGRAM:
      g_backing[addr] = val & 0xff;
      break;
    case CYCLE_CFI:
      break;
  }
  
}


// word width write
static void write_w2(uint32_t addr, uint32_t val)
{

  switch (g_state)
  {
    case CYCLE_READ:
      if ((addr==0xaaa) && (val==0xaa)) {
        g_state = CYCLE_1;
      } else if ((addr==0xaa) && (val==0x9898)) {
        g_state = CYCLE_CFI;
      } else if ((addr==0xaa) && (val==0x98)) {
	// alternative
        g_state = CYCLE_CFI;
      } else if ((addr==0xaaa) && (val==0x98)) {
	// alternative
        g_state = CYCLE_CFI;
      }
      break;
    case CYCLE_1:
      if ((addr==0x554) && (val==0x55)) {
        g_state = CYCLE_2;
      }
      break;    
    case CYCLE_2:
      if (addr==0xaaa)
      {
        switch (val)
        {
          case 0x90:
            g_state = CYCLE_AUTOSELECT;
            break;
          case 0xa0:
            g_state = CYCLE_PROGRAM;
            break;
          case 0x80:
            g_state = CYCLE_3;
            break;
        }
      }
      break;
    case CYCLE_3:
      if ((addr==0xaaa) && (val==0xaa)) {
        g_state = CYCLE_4;
      }
      break;    
    case CYCLE_4:
      if ((addr==0x554) && (val==0x55)) {
        g_state = CYCLE_ERASE;
      }
      break;    
    case CYCLE_ERASE:
      if (val==0x30) {
	EraseSector(addr);
      } else if (val==0x10) {
	ChipErase();
      }
      break;    
    case CYCLE_AUTOSELECT:
      break;
    case CYCLE_PROGRAM:
      *((unsigned short*)(&g_backing[addr])) = val & 0xffff;
      break;
    case CYCLE_CFI:
      break;
  }
  

}



static void flash_write_width(void *opaque, target_phys_addr_t addr, uint32_t value, int width)
{
  enum FLASH_STATE before = g_state;

  
  switch (width)
  { 
    case 1:
      write_w1(addr, value);
      break;
    case 2:
      write_w2(addr, value);
      break;
  }      
  
  // return to read mode on any divergence from expected.
  if (before == g_state)
    g_state = CYCLE_READ;

  //printf("Write:  0x%x, 0x%x, %d  (state %s)\n", addr, value, width, state2string(g_state));
}


// Return the CFI data matching 
unsigned short g_CFI[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // up to 0x0f unspecified 
        0x51, 0x52, 0x59, // 'QRY' ascii string
        2, 0, // Primary OEM command set
        0x40, 0, // address for primary extended table
        0, 0, // OEM command set (0 == none)
        0, 0, // address for OEM extended table (0 == none)
        0x27, // Vcc min (write/erase)
        0x36, // Vcc max (write/erase)
        0, 0, // Vpp Min/Max
        4, 0, 0xa, 0, 5, 0, 4, 0,  // timeouts
        0x17, // device size (2^N)
        2, 0, // interface description
        0, 0, // max byte in multi-byte write (0== not supported)
        2, // number of erase block regions
        7, 0, 0x20, 0, // erase block region 1 info.
        0x7e,0,0,1,  // erase block region 2 info
        0,0,0,0,  // region 3
        0,0,0,0,  // region 4
        0,0,0,   // padding to 0x40h.
        0x50, 0x52, 0x49,
        0x31, 0x31,
        0, 2, 4, 1, 4, 0, 0, 0, 0xa5, 0xc5, 2
};


static uint32_t flash_read_width(void *opaque, target_phys_addr_t addr, int width)
{
  uint32_t ret;
  unsigned char* tmp = g_backing;
   
  if (g_state == CYCLE_AUTOSELECT)
  {
    if (addr == 0x000)
      return 0x7f;
    else if ((addr == 0x100) || (addr == 0x200))
      return 0x1c;
    else if (((addr & 0x0ff) == 0x01) || ((addr & 0x0ff) == 0x02))
      return 0x22cb;
    else
      return 0;
  }

  if (g_state == CYCLE_CFI)
  {
    if (addr<sizeof(g_CFI))
      tmp = (unsigned char*)g_CFI;
  }
  
  tmp += addr;
  
  switch (width)
  { 
    case 1:
      ret = *((unsigned char*)tmp);
      break;
    case 2:
      ret = *((unsigned short*)tmp);      
      break;
    case 4:
      ret = *((unsigned long*)tmp);      
      break;
    default:
      ret = 0;
  }      

  //printf("Read:  0x%x, %d, (ret: %x)  (state %s)\n", addr, width, ret, state2string(g_state));
  return ret;
}



static void flash_writeb(void *opaque, target_phys_addr_t addr, uint32_t value)
{ flash_write_width(opaque, addr, value, 1); }
static void flash_writew(void *opaque, target_phys_addr_t addr, uint32_t value)
{ flash_write_width(opaque, addr, value, 2); }
static void flash_writel(void *opaque, target_phys_addr_t addr, uint32_t value)
{ flash_write_width(opaque, addr, value, 4); }

static uint32_t flash_readb(void *opaque, target_phys_addr_t addr)
{ return flash_read_width(opaque, addr, 1); }
static uint32_t flash_readw(void *opaque, target_phys_addr_t addr)
{ return flash_read_width(opaque, addr, 2); }
static uint32_t flash_readl(void *opaque, target_phys_addr_t addr)
{ return flash_read_width(opaque, addr, 4); }

static CPUReadMemoryFunc *flash_read_mem[] = { &flash_readb, &flash_readw,  &flash_readl };
static CPUWriteMemoryFunc *flash_write_mem[] = { &flash_writeb, &flash_writew, &flash_writel };


static void en29lv640_register(void)
{
  ram_addr_t index;
  flash_load_image(firmware_name);
  
  g_state = CYCLE_READ;  // initial state
    
  // register a fake device that simply cannot erase the top (bios) sector.  We'll write
  // a warning if they try.  There's little point in supporting the user hosing the system.
  index = cpu_register_io_memory(flash_read_mem, flash_write_mem, NULL, DEVICE_LITTLE_ENDIAN);
  cpu_register_physical_memory(FLASH_BASE, FLASH_DEV_LEN, index);
}


device_init(en29lv640_register)

