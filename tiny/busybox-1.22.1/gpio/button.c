/* vi: set sw=4 ts=4: */
/*
 * Button busybox applet.
 *
 * Copyright (c) Bifferos (sales@bifferos.com)
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * 
 * The most obvious use of this applet is to allow a 'reset' button to reset 
 * the device, something that would normally have to be built into the main
 * device application or other custom server.
 *
 * Special thanks to Radoslav Kolev for the GPIO code.
 */


//usage:#define button_trivial_usage
//usage:       "[-t <delay>] [-F] BUTTON HELPER"
//usage:#define button_full_usage "\n\n"
//usage:       "Execute HELPER when button BUTTON is pressed\n"
//usage:       "\n        -t N    Set the sampling interval (mS)"
//usage:       "\n        -h      Exec when gpio goes high (default: low)"
//usage:       "\n        -F      Run in foreground"
//usage:       "\n"
//usage:       "\nHELPER receives the button number as argv[1], count as argv[2]"
//usage:#define button_example_usage
//usage:       "# button\n"
//usage:       "# button 15 /etc/button.sh\n"


#include <syslog.h>
#include "libbb.h"


#define MAX_BUTTON_STRING 20    /* button string */
#define MAX_GPIO_STRING 60      /* complete path to button


struct globals {
	char gpio_button[MAX_BUTTON_STRING];
	char gpio_pin_path[MAX_GPIO_STRING];	
} FIX_ALIASING;


#define G (*(struct globals*)&bb_common_bufsiz1)


static void linuxgpio_export(int export)
{
	int fd;
	char buf[MAX_GPIO_STRING];
	char* butt_str = G.gpio_button;

	fd = xopen(export?"/sys/class/gpio/export":"/sys/class/gpio/unexport", O_WRONLY);
	write(fd, butt_str, strlen(butt_str));
	close(fd);

	if (export)
	{
		/* set direction */
		snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%s/direction", G.gpio_button);
		fd = xopen(buf, O_WRONLY);
		write(fd, "in", 3);
		close(fd);
	}
}


static int linuxgpio_getpin(void)
{
	char c;
	int fd;

	fd = xopen(G.gpio_pin_path, O_RDWR);
	
	if (read(fd, &c, 1)!=1)
		bb_perror_msg_and_die("Failed to read pin");

	close(fd);
	
	if (c=='0')	return 0;
	if (c=='1') return 1;
	
	bb_perror_msg_and_die("Pin value");
}



static void button_shutdown(int sig UNUSED_PARAM)
{
	linuxgpio_export(0);
	_exit(EXIT_SUCCESS);
}


static void run_helper(char* path, char* count)
{
	char *const args[] = {path, G.gpio_button, count, NULL};
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


int button_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int button_main(int argc, char **argv )
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

	strncat(G.gpio_button, argv[argc - 2], sizeof(G.gpio_button)-1);
	snprintf(G.gpio_pin_path, sizeof(G.gpio_pin_path), "/sys/class/gpio/gpio%s/value", G.gpio_button);

	linuxgpio_export(1);
  
	bb_signals(BB_FATAL_SIGS, button_shutdown);
       
	bb_info_msg("Button on GPIO %s, period %d mS\n", G.gpio_button, sample_period);
  
	sample_period *= 1000;  // mS
  
	while (1)
	{
		usleep(sample_period);
    
		if (press == linuxgpio_getpin())
		{
			bb_info_msg("Button %s pressed (%d)\n", G.gpio_button, push_count);
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



