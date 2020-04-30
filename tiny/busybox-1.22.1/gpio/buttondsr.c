/* vi: set sw=4 ts=4: */
/*
 * Buttondsr busybox applet.
 *
 * Copyright (c) Bifferos (sales@bifferos.com)
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * 
 * Run a program when dsr goes low on given serial port.
 */


//usage:#define buttondsr_trivial_usage
//usage:       "[-t <delay>] [-F] SERIALDEV HELPER"
//usage:#define buttondsr_full_usage "\n\n"
//usage:       "Execute HELPER when SERIALDEV DSR is taken low\n"
//usage:       "\n        -t N    Set the sampling interval (mS)"
//usage:       "\n        -h      Exec HELPER when dsr goes high (default: low)"
//usage:       "\n        -F      Run in foreground"
//usage:       "\n"
//usage:       "\nHELPER receives the count as argv[1]"
//usage:#define buttondsr_example_usage
//usage:       "# buttondsr\n"
//usage:       "# buttondsr /dev/ttyUSB0 /etc/button.sh\n"


#include <syslog.h>
#include "libbb.h"


#define MAX_BUTTON_DEVICE_STRING 80    /* button string */


struct globals {
	char tty_device_path[MAX_BUTTON_DEVICE_STRING];
	int fd_tty;
} FIX_ALIASING;


#define G (*(struct globals*)&bb_common_bufsiz1)


static void serial_open(const char* devicename)
{
  int fd = -1;

  printf("Waiting for serial port to become available\n");
  while (access(devicename, F_OK)!=0)
  {
    sleep(1);
  }

  printf("Waiting for access to port\n");
  while (1)
  {
    fd = open(devicename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd>=0) break;
    perror("Waiting for port:");
  }
  
  G.fd_tty = fd;
}


static int serial_getdsr(void)
{
  int s;
  
  if (ioctl(G.fd_tty, TIOCMGET, &s))
  {
    perror("getting dsr:");
    exit(1);
  }
  
  if (s & TIOCM_DSR)
  {
     return 0;
  }
  else
  {
     return 1;
  }
}


static void serial_shutdown(int sig UNUSED_PARAM)
{
	close(G.fd_tty);
	_exit(EXIT_SUCCESS);
}


static void run_helper(char* path, char* count)
{
	char *const args[] = {path, G.tty_device_path, count, NULL};
	if (execvp(path,args)==-1)
		bb_perror_msg_and_die("Failed To execute action");
	_exit(EXIT_SUCCESS);
}


static void fork_helper(char* path, int count)
{
	char count_str[10];
	int pid = fork();

	if (0 == pid)
	{
		snprintf(count_str, sizeof(count_str), "%d", count);
		run_helper(path, count_str);
	}
	else
	{
		wait4pid(pid);
	}
}


#define OPT_FOREGROUND    (1 << 0)
#define OPT_HIGH_PRESS    (1 << 1)
#define OPT_SAMPLEPERIOD  (1 << 2)
#define FOREGROUND (opts & OPT_FOREGROUND)


int buttondsr_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int buttondsr_main(int argc, char **argv )
{
	int press = 0;
	int push_count = 0;
	unsigned opts;
	char* sample_arg;
	unsigned sample_period = 100;
  
	//INIT_G();
  
	opt_complementary = "=2";  /* 2 arguments+ */
	opts = getopt32(argv, "Fht:", &sample_arg);

  
	if (!FOREGROUND)
	{
		bb_daemonize_or_rexec(DAEMON_CHDIR_ROOT, argv);
	}
  
	if (!FOREGROUND)
	{
		openlog(applet_name, LOG_PID | LOG_NDELAY, LOG_DAEMON);
		logmode |= LOGMODE_SYSLOG;
	}
	
	if (opts & OPT_HIGH_PRESS)
	{
		press = 1;
	}

	if (opts & OPT_SAMPLEPERIOD)
	{
		sample_period = xatoi_positive(sample_arg);
	}

	strncat(G.tty_device_path, argv[argc - 2], sizeof(G.tty_device_path)-1);
	
	serial_open(G.tty_device_path);
	bb_signals(BB_FATAL_SIGS, serial_shutdown);
       
	bb_info_msg("Checking DSR on %s, period %d mS\n", G.tty_device_path, sample_period);
  
	sample_period *= 1000;  // mS
  
	while (1)
	{
		usleep(sample_period);
    
		if (press == serial_getdsr())
		{
			bb_info_msg("DSR on %s changed (%d)\n", G.tty_device_path, push_count);
			fork_helper(argv[argc - 1], push_count);
			push_count++;
		}
		else
		{
			push_count = 0;
		}
	}  
	return 0;
}
