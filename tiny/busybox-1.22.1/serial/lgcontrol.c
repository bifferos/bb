

//usage:#define lgcontrol_trivial_usage
//usage:       "<device> <codes>"
//usage:#define lgcontrol_full_usage "\n\n"
//usage:       "Send control codes to LG TV\n"
//usage:#define lgcontrol_example_usage
//usage:       "# lgcontrol /dev/ttyUSB0 ka 00 01\n"


#include "libbb.h"


#include <time.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>


static int serial_open(const char* devicename)
{
  int fd = -1;
  while (access(devicename, F_OK)!=0)
  {
    sleep(1);
  }
  
  while (1)
  {
    fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd>=0) break;
    perror("Waiting for port:");
  }
  
  return fd;
}


static void serial_speed(int fd)
{
  struct termios c;
  int ret;
  tcgetattr(fd, &c);
  cfsetispeed(&c, B9600);
  cfsetospeed(&c, B9600);
  c.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  
  c.c_iflag = 0;  // raw input
  c.c_oflag = 0;  // raw output
   
  c.c_lflag = 0;  // NO CANON
  
  c.c_cc[VINTR]    = 0;     // Ctrl-c  
  c.c_cc[VQUIT]    = 0;     // Ctrl-slash 
  c.c_cc[VERASE]   = 0;     // del 
  c.c_cc[VKILL]    = 0;     // @ 
  c.c_cc[VEOF]     = 0;     // Ctrl-d 
  c.c_cc[VTIME]    = 0;     // inter-character timer unused 
  c.c_cc[VMIN]     = 1;     // blocking read until 1 character arrives 
  c.c_cc[VSWTC]    = 0;     // '\0' 
  c.c_cc[VSTART]   = 0;     // Ctrl-q  
  c.c_cc[VSTOP]    = 0;     // Ctrl-s 
  c.c_cc[VSUSP]    = 0;     // Ctrl-z 
  c.c_cc[VEOL]     = 0;     // '\0' 
  c.c_cc[VREPRINT] = 0;     // Ctrl-r 
  c.c_cc[VDISCARD] = 0;     // Ctrl-u 
  c.c_cc[VWERASE]  = 0;     // Ctrl-w 
  c.c_cc[VLNEXT]   = 0;     // Ctrl-v 
  c.c_cc[VEOL2]    = 0;     // '\0' 

  tcflush(fd, TCIFLUSH);

  ret = tcsetattr(fd, TCSANOW, &c);

  if (ret!=0)
  {  
    perror("Unable to set speed");
    _exit(-1);
  }
}


static int serial_wait(int fd)
{
  fd_set master_set;
  fd_set read_set;
  char out;
  struct timeval timeout;
  
  /* Initialize the file descriptor set */
  FD_ZERO(&master_set);
  FD_SET(fd, &master_set);
  
  for (int i=0; i<16; i++)
  {
    read_set = master_set;
    
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (select(FD_SETSIZE, &read_set, NULL, NULL, &timeout) < 0)
    {
      perror("select");
      return 1;
    }
    
    if (FD_ISSET(fd, &read_set))
    {
      read(fd, &out, 1);
      putchar(out);
      if (out == 'x')
      {
        putchar('\n');
        break;
      }
    }
    else
    {
      // timeout
      printf("serial timeout\n");
      return 1;
    }    
  }  
  return 0;
}


static int Help(void)
{
  printf("Usage: <dev> <command> <set id> <data>\n");
  _exit(0);
}


int lgcontrol_main(int argc, char** argv) MAIN_EXTERNALLY_VISIBLE;
int lgcontrol_main(int argc, char** argv)
{
  int fd;
  const char space=' ';
  const char newline=0x0d;
  
  if (argc != 5) Help();
  
  fd = serial_open(argv[1]);
  serial_speed(fd);
  
  write(fd,argv[2], strlen(argv[2]));
  write(fd,&space,1);
  write(fd,argv[3], strlen(argv[3]));
  write(fd,&space,1);
  write(fd,argv[4], strlen(argv[4]));
  write(fd,&newline,1);
  
  return serial_wait(fd);
}

