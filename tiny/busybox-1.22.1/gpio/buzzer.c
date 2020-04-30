/* vi: set sw=4 ts=4: */
/*
 * Buzzer busybox applet.
 *
 * Copyright (c) Bifferos (sales@bifferos.com)
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * 
 * Changes serial control line
 */


//usage:#define buzzer_trivial_usage
//usage:       "[-t <delay>] SERIALDEV NAME [1|0]"
//usage:#define buzzer_full_usage "\n\n"
//usage:       "Pull serial output line low for given period\n"
//usage:       "\n        -t N    Set the period for the sounding (mS)"
//usage:       "\n"
//usage:       "\nHELPER receives the count as argv[1]"
//usage:#define buzzer_example_usage
//usage:       "# buzzer\n"
//usage:       "# buzzer /dev/ttyUSB0 TIOCM_RTS 1\n"


#include <syslog.h>
#include "libbb.h"


static int serial_open(const char* devicename)
{
  int fd = -1;

  fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0)
  {
    bb_perror_msg_and_die("Opening serial port");
  }
  
  return fd;
}


static int convert_parameter(char* param)
{
  if (strcmp(param, "TIOCM_DTR") == 0)
  {
    return TIOCM_DTR;
  }
  else if (strcmp(param, "TIOCM_RTS") == 0)
  {
    return TIOCM_RTS;
  }
  bb_error_msg_and_die("Unknown serial port control value");
}


static void serial_set_param(int fd, int param, int value, int time_period)
{
  int original_value;
  int new_value;
  
  if (ioctl(fd, TIOCMGET, &original_value))
  {
    perror("getting serial port state:");
    exit(1);
  }
    
  if (value)
  {
    new_value = original_value | param;  
  }
  else
  {
    new_value = original_value & ~param;
  }

  if (ioctl(fd, TIOCMSET, &new_value))
  {
    perror("setting serial port state:");
    exit(1);
  }
  
  usleep(time_period);
  
  if (ioctl(fd, TIOCMSET, &original_value))
  {
    perror("restoring serial port state:");
    exit(1);
  }  
}


#define OPT_TIMEPERIOD  (1 << 0)


int buzzer_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int buzzer_main(int argc, char **argv )
{
        int serial_fd;
	unsigned opts;
	char* time_arg;
	int parameter;
	int value;
	unsigned time_period = 1000;
    
	opt_complementary = "=3";  /* 2 argument */
	opts = getopt32(argv, "t:", &time_arg);

	if (opts & OPT_TIMEPERIOD)
	{
		time_period = xatoi_positive(time_arg);
	}
	
	serial_fd = serial_open(argv[argc-3]);
	parameter = convert_parameter(argv[argc-2]);
	value = xatoi_positive(argv[argc-1]);

	time_period *= 1000;  // mS
	
	serial_set_param(serial_fd, parameter, value, time_period);
    
	return 0;
}
