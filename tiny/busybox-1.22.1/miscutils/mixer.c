/* vi: set sw=4 ts=4: */
/*
 * Audio mixer for busybox (OSS emulation only)
 *
 * Copyright (c) Bifferos (sales@bifferos.com)
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 * 
 * Setting L/R stereo channels independently is unsupported in order to keep this applet small.
 * 
 *
 */


#include <linux/soundcard.h>
#include "libbb.h"


//usage:#define mixer_trivial_usage
//usage:       "[-d DEVICE] [MIXER [LEVEL]]"
//usage:#define mixer_full_usage "\n\n"
//usage:       "Sets the mixer volume at a specific level (0-100)\n"
//usage:       "\n        -d DEVICE    use given mixer device (default /dev/mixer)"
//usage:#define mixer_example_usage
//usage:       "# mixer vol 50\n"


static void pr_level_exit(int level)
{
	printf("Level (L/R): %d/%d\n", level&0xff, ((level&0xff00) >> 8));
	exit(0);
}


int mixer_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int mixer_main(int argc, char **argv )
{
	int fd_mixer;
	unsigned opts, extra_args;
	char* device;
	char* mixer = 0;
	uint32_t write_level;
	uint32_t read_level;
	int devmask, i;
	const char *m_names[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES ;
	int mixer_device=0;


	/* Option handling */
	
	opt_complementary = "?2";  /* must have no more than 2 */
	opts = getopt32(argv, "d:", &device);

	extra_args = argc - 1;
	if (opts & 1)
	{
		fd_mixer = xopen(device, O_RDWR);
		extra_args -= 2;
	}
	else
	{
		fd_mixer = xopen("/dev/mixer", O_RDWR);
	}
	
	if (extra_args) mixer = argv[argc - ((extra_args==1)?1:2)];


	/* mixer device enumeration */

	ioctl_or_perror_and_die(fd_mixer, SOUND_MIXER_READ_DEVMASK, &devmask, "DEVMASK");
	
	if (!mixer) printf("Mixer:");

	for (i = 0; i < SOUND_MIXER_NRDEVICES; ++i)
	{
		if ((1 << i) & devmask) {
			if (mixer)
			{
				if (strcmp(m_names[i], mixer) == 0) mixer_device = i;
			}
			else
			{
				printf(" %s", m_names[i]);
			}
		}
	}

	if (!mixer)
	{
		printf("\n");
		exit(0);
	}


	/* mixer device reading */

	ioctl_or_perror_and_die(fd_mixer, MIXER_READ(mixer_device), &read_level, "MIXER_READ");

	if (extra_args == 1) pr_level_exit(read_level);


	/* mixer device setting */

	write_level = xatou_range(argv[argc - 1], 0, 100);
	write_level = (write_level<<8) + write_level;

	ioctl_or_perror_and_die(fd_mixer, MIXER_WRITE(mixer_device), &write_level, "MIXER_WRITE");

	pr_level_exit(write_level);
	

	return 0;
}
