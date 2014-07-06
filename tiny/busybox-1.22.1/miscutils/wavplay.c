/* vi: set sw=4 ts=4: */
/*
 * wavplay for busybox
 *
 * Copyright (c) Bifferos (sales@bifferos.com)
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 *
 */


#include <linux/soundcard.h>
#include "libbb.h"


//usage:#define wavplay_trivial_usage
//usage:       "[-d DEVICE] WAVFILE"
//usage:#define wavplay_full_usage "\n\n"
//usage:       "Plays a .WAV file to OSS pcm device\n"


typedef struct wav_header
{
	char      id[4];          // should always contain "RIFF"
	uint32_t  totallength;    // total file length minus 8
	char      wavefmt[8];     //  "WAVEfmt "
	uint32_t  format;         // 16 for PCM
	uint16_t  pcm;            // 1 for PCM 
	uint16_t  channels;       // # channels
	uint32_t  frequency;      // frequency
	uint32_t  byte_rate;      // byte rate
	uint16_t  blockAlign;     // not checked
	uint16_t  bits_per_sample;
	char      data[4];        // "data"
	uint32_t  bytes_in_data;
} wav_header;


int wavplay_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int wavplay_main(int argc, char **argv )
{
	wav_header h;
	int tmp;
	int fd_wav, fd_dsp;
	size_t buffer_len=0;
	void* buffer=0;
	size_t read_result = 1;
	size_t write_result;
	unsigned opts;
	char* device;
  
	opt_complementary = "=1";  /* must have exactly 1 argument */
	opts = getopt32(argv, "d:", &device);

	fd_wav = xopen(argv[argc - 1], O_RDONLY);
	fd_dsp = xopen((opts & 1)?device:"/dev/dsp", O_RDWR);
	
	if (  (read(fd_wav, &h, sizeof(h)) != sizeof(h)) ||
			memcmp(h.id, "RIFF", 4) ||  
			memcmp(h.wavefmt, "WAVEfmt \x10\x00\x00\x00\x01\x00", 14) ||
			memcmp(h.data, "data", 4)
           )
	{
	    bb_error_msg_and_die("Format not recognised\n");
	}
	printf("Freq: %d Chs: %d Bits: %d\n", h.frequency, h.channels, h.bits_per_sample);
    
	tmp = h.bits_per_sample;
	ioctl_or_perror_and_die(fd_dsp, SOUND_PCM_WRITE_BITS, &tmp, "bits");
	tmp = h.channels;
	ioctl_or_perror_and_die(fd_dsp, SOUND_PCM_WRITE_CHANNELS, &tmp, "channels");
	ioctl_or_perror_and_die(fd_dsp, SOUND_PCM_WRITE_RATE, &h.frequency, "rate");
		
	// allocate a block capable of playing for 1/4 second.
	buffer_len = h.byte_rate/4;     
	buffer = xmalloc(buffer_len);
		
	while (read_result)
	{
		read_result = read(fd_wav, buffer, buffer_len);
		if (read_result == -1) bb_perror_msg_and_die("Reading WAV");
		write_result = write(fd_dsp, buffer, read_result);
		if (write_result != read_result) bb_perror_msg_and_die("write truncated");
	}
	return 0;
}
