
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <stdlib.h>

#define LCD_MESSAGE "/var/lcd_message.txt"
#define TEMP_FILE "/var/ch_temperature.txt"
#define THERM_FILE "/var/ch_thermostat.txt"


void send_char(int ser_fd, unsigned char d)
{
  unsigned char byte;
  byte = d;
  write(ser_fd, &byte, 1);  
}


void lcd_line1(int ser_fd)
{
  send_char(ser_fd, '<');
}

void lcd_line2(int ser_fd)
{
  send_char(ser_fd, '>');
}

void check_messages(int ser_fd)
{
  int fd = open(LCD_MESSAGE, O_RDONLY);
  if (fd < 0) return;  // no messages

  lcd_line2(ser_fd);

  while (1)
  {
    unsigned char byte;
    ssize_t count = read(fd, &byte, 1);
    if (!count) break;
    write(ser_fd, &byte, 1);
  }
  close(fd);
  unlink(LCD_MESSAGE);
}


static float g_temperature_now = 15.0;


float string_to_temp(char* txt)
{
  float tmp;
  int count = sscanf(txt, "%f", &tmp);
  if (count != 1)
    tmp = 66.0;
  return tmp;
}


void get_temperature(int ser_fd)
{
  unsigned char byte;
  byte = '?';
  write(ser_fd, &byte, 1);

  sleep(1);  // wait for response
  char buffer[20];
  ssize_t count = read(ser_fd, &buffer[0], sizeof(buffer)-1);
  if (count<1)
  {
    perror("Serial read error");
    return;
  }
  if (count)
  {
    //printf("Received %d bytes from serial port\n", count);
    buffer[count] = 0;
    unlink(TEMP_FILE);
    int fd = open(TEMP_FILE, O_CREAT|O_RDWR, 0755);
    if (fd < 0)
    {
      printf("Failed to open file '%s'\n", TEMP_FILE);
      return;
    }
    write(fd, &buffer, count);
    //printf("Written %d bytes to file\n", written);
    close(fd);

    g_temperature_now = string_to_temp(buffer);
  }
  else
  {
    printf("No bytes read back from serial port\n");
    if (access(TEMP_FILE, F_OK) == 0)
    {
      unlink(TEMP_FILE);
    }
  }
}


static float g_thermostat_temperature = 10.0;

// set the above value from file.
void get_thermostat()
{
  FILE* fp = fopen(THERM_FILE, "r");
  if (fp)
  {
    fscanf(fp, "%f", &g_thermostat_temperature);
    //printf("read thermostat, %0.1f\n", g_thermostat_temperature);
    fclose(fp);
  }
  else
  {
    g_thermostat_temperature = 10.0;
    FILE* fp = fopen(THERM_FILE, "w");
    if (fp)
    {
      fprintf(fp, "10.0");
      fclose(fp);
    }
  }
}


static bool g_last_state = false;   // relay off at program start.


void update_relay(int ser_fd)
{
  // implement some hysterisis
  bool new_state = g_last_state;

  if (g_last_state)
  {
    // needs to go to 0.5 degrees over the target temp before switch off.
    if (g_temperature_now > (g_thermostat_temperature + 0.5))
    {
      // switch off
      new_state = false;
    }
  }
  else
  {
    // needs to go to 0.5 degrees over the target temp before switch off.
    if (g_temperature_now < (g_thermostat_temperature - 0.5))
    {
      // switch on
      new_state = true;
    }
  }
  if (new_state != g_last_state)
  {
    if (new_state)
    {
      send_char(ser_fd, '+');
    }
    else
    {
      send_char(ser_fd, '-');
    }
    g_last_state = new_state;
  }
}


void update_display(int ser_fd)
{
  lcd_line1(ser_fd);
  char buffer[20];
  sprintf(buffer, "%0.2f (%0.1f) %s", g_temperature_now, g_thermostat_temperature, g_last_state?"on ":"off");

  write(ser_fd, &buffer[0], strlen(buffer));
}



void arduino_speed(int fd)
{
  printf("Setting speed\n");
  struct termios c;
  tcgetattr(fd, &c);
  cfsetispeed(&c, B9600);
  cfsetospeed(&c, B9600);
  c.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
  c.c_iflag = 0;  // raw input
  c.c_oflag = 0;  // raw output
 
  c.c_lflag = ICANON;
  
         c.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
         c.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
         c.c_cc[VERASE]   = 0;     /* del */
         c.c_cc[VKILL]    = 0;     /* @ */
         c.c_cc[VEOF]     = 0;     /* Ctrl-d */
         c.c_cc[VTIME]    = 0;     /* inter-character timer unused */
         c.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
         c.c_cc[VSWTC]    = 0;     /* '\0' */
         c.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
         c.c_cc[VSTOP]    = 0;     /* Ctrl-s */
         c.c_cc[VSUSP]    = 0;     /* Ctrl-z */
         c.c_cc[VEOL]     = 0;     /* '\0' */
         c.c_cc[VREPRINT] = 0;     /* Ctrl-r */
         c.c_cc[VDISCARD] = 0;     /* Ctrl-u */
         c.c_cc[VWERASE]  = 0;     /* Ctrl-w */
         c.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
         c.c_cc[VEOL2]    = 0;     /* '\0' */

  tcflush(fd, TCIFLUSH);

  int ret = tcsetattr(fd, TCSANOW, &c);

  if (ret!=0)
  {  
    perror("Unable to set speed of arduino");
    _exit(-1);
  }
}



char* basename(char* path)
{
  char* ptr;
  ptr = strrchr(path, '/');
  if (ptr)
  {
    ptr++;
  }
  else
  {
    ptr = path;
  }
  return ptr;
}



int main_chmon(int argc, char* argv[])
{
  if (argc!=2)
  {
    printf("Usage: chmon <port>\n");
    _exit(-1);
  }

  const char* devicename = argv[1];

  printf("Waiting for port to become available\n");
  while (access(devicename, F_OK)!=0)
  {
    sleep(1);
  }

  int fd = -1;
  printf("Waiting for access to port\n");
  while (1)
  {
    fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd>=0) break;
    perror("Waiting for port:");
  }
  
  arduino_speed(fd);

  // enter the main loop
  while (1)
  {
    // check for messages in the message file
    check_messages(fd);
    
    // get temperature and write it to file.
    get_temperature(fd);
    
    // read in the thermostat temperature
    get_thermostat();
    // update relay
    update_relay(fd);
    // update display
    update_display(fd);
  }
  return 0;
}



void decode(char* src, char* dest)
{
  while (*src)
  {
    if(*src == '+')
      *dest = ' ';
    else if(*src == '%') {
      int code;
      if(sscanf(src+1, "%2x", &code) != 1) code = '?';
      *dest = code;
      src +=2; 
    }     
    else
      *dest = *src;
    src++;
    dest++;
  }
  ++dest;
  *dest = 0;
}


int main_message(int argc, char* argv[])
{
  char* data = getenv("QUERY_STRING");

  printf("Content-type:text/html\n\n");
  printf("<html><body>\n");

  if (data)
  {
    char encoded[1024];
    char unencoded[1024];
    if (strlen(data) > 1000) _exit(-1);
    sscanf(data, "message=%s", encoded);
    // no point in accepting more than 16 chars

    decode(encoded, unencoded);

    while (strlen(unencoded)<16)
    {
      strcat(unencoded, " ");
    }
    
    unlink(LCD_MESSAGE);

    int fd = open("/tmp/lcd_message.txt", O_CREAT|O_RDWR, 0755);
    if (fd < 0)
    {
      printf("Failed to open tmp file for message\n");
      return -1;  // server error
    }
    write(fd, &unencoded[0], strlen(unencoded));
    //printf("Written %d bytes to file\n", written);
    close(fd);

    rename("/tmp/lcd_message.txt", LCD_MESSAGE);
    printf("Message '%s' sent to LCD\n", unencoded);
  }
  else
  {
    printf("Error: missing message text\n");
  }

  printf("</body></html>\n");
  return 0;
}


int main_therm(int argc, char* argv[])
{
  char* data = getenv("QUERY_STRING");

  printf("Content-type:text/html\n\n");
  printf("<html><body>\n");

  if (data)
  {
    int temp = 10;
    printf("Data: '%s'\n", data);
    sscanf(data, "therm=%d", &temp);
    
    int fd = open("/tmp/ch_thermostat.txt", O_CREAT|O_RDWR, 0755);
    if (fd < 0)
    {
      printf("Failed to open tmp file for thermostat setting\n");
      return -1;  // server error
    }
    char buffer[20];
    sprintf(buffer, "%0.1f\n", float(temp));
    write(fd, &buffer[0], strlen(buffer));
    close(fd);

    rename("/tmp/ch_thermostat.txt", THERM_FILE);
    printf("Thermostat set to: %d degrees", temp);
  }
  else
  {
    printf("Error: missing thermostat setting\n");
  }

  printf("</body></html>\n");
  return 0;
}



int main(int argc, char* argv[])
{

  char* bn = basename(argv[0]);
  if (strcmp(bn, "chmon")==0)
  {
    return main_chmon(argc, argv);
  } 
  else if (strcmp(bn, "message")==0)
  {
    return main_message(argc, argv);
  }
  else if (strcmp(bn, "therm")==0)
  {
    return main_therm(argc, argv);
  }
  printf("Unknown applet\n");
  return -1;

}
