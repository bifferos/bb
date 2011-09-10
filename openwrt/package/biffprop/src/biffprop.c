//
// License:  Public domain, use for any purpose allowed, commercial or otherwise.
// If you want to make a donation, buy a Bifferboard, or pursuade a friend to :).
//
// CREDITS:  Owes much to Loader.py by Remy Blank, since that's the first 
// implementation I found in a language I could understand!
// Thanks also to BradC for helpful suggestions.
//
// Requires a 'real' UART, tested on 16550, uses an RDC GPIO pin for the RST
// line, 'cos Bifferboard DTR ain't available.  Patches for command-line option 
// to switch between using DTR or linux GPIO pin for this welcomed.
//
// bifferos@yahoo.co.uk (www.bifferos.com).
//


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/io.h>


#define CONTROL 0x80003848
#define DATA 0x8000384c

#define BIT_RST  (1<<12)   // GPIO 12 

#define RST_LOW  outl(0, 0xcfc);    // start off with it high.
#define RST_HIGH  outl(BIT_RST, 0xcfc);   

#define CMD_SHUTDOWN       0
#define CMD_RUN            1


typedef unsigned long u32;
typedef unsigned char u8;

// init the lfsr array
static unsigned char lfsr_data[500];

void lfsr_init()
{
  int i;
  u8 seed = 'P';
  for (i=0;i<sizeof(lfsr_data);i++)
  {
    lfsr_data[i] = seed & 0x01;
    seed = ((seed << 1) & 0xfe) | (((seed >> 7) ^ (seed >> 5) ^ (seed >> 4) ^ (seed >> 1)) & 1);
  }
}


static void tx_byte(u8 p)
{
  while ((inb(0x3f8+5) & (1<<5))==0);
  outb(p, 0x3f8);
}


static int rx_ready()
{
  return (inb(0x3f8+5) & 1);
}

static u8 rx_byte(void)
{
  while (1)
  {
    if (inb(0x3f8+5) & 1)
      return inb(0x3f8);
  }
}


// assumes buffer large enough, returns bytes copied (to transmit)
static void writeLong(u32 value)
{
  int i;
  u8 tmp;
  for (i=0;i<10;i++)
  {
    tmp = 0x92 | (value & 0x01) | ((value & 2) << 2) | ((value & 4) << 4);
    tx_byte(tmp);
    value >>=3;
  }
  tmp = 0xf2 | (value & 0x01) | ((value & 2) << 2);
  tx_byte(tmp);
}


void mdelay(u32 ms)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = ms * 1000000;    // 15mS.
  nanosleep(&t, &t);
}


int prop_connect()
{
  int i;
  u8 received, version;
  
  lfsr_init();

//  RST_HIGH;
//  mdelay(15);
  
  // BradC 'double-reset' routine:
//  RST_LOW;
//  mdelay(15);  
//  RST_HIGH;
//  mdelay(15);
  RST_LOW;
  mdelay(15);
  RST_HIGH;
  mdelay(90);    // BradC thinks thi should be 95.

  tx_byte(0xf9);  // calibration pulse

  // Connect to the 
  for (i=0;i<(sizeof(lfsr_data)/2);i++)
  {
    tx_byte(lfsr_data[i] | 0xfe);
  }
  
  // What is this for?
  i = 250;
  while (i<sizeof(lfsr_data))
  {
    tx_byte(0xf9);  // for pacing
    received = rx_byte();
    // check validity
    if ((lfsr_data[i] | 0xfe)!= received)
    {
      // Just print out the error, don't abort, to see what's going on.
      // Should probably store these up rather than allow the console output to 
      // interfere with timing, after all it might be over a network.
      printf("Error in data stream at %d %x  (%d)\n", i, received, lfsr_data[i]);
      // Should really break here, but I prefer the diagnostic.
//      break;
    }
    i++;
  }

  // version bit.
  
  version = 0;
  for (i=0;i<8;i++)
  {
    tx_byte(0xf9);
    version >>= 1;
    received = rx_byte();
    version |= (received == 0xff) ? 0x80 : 0;
  }
    
  return version;
}



void prop_upload(u8* buffer, size_t len)
{
  u32* ptr = (u32*)buffer;
  u32 count = len/4;    // assume this is multiple of 4 and no padding needed.
  u8 response;
  writeLong(CMD_RUN);
  writeLong(count);
  while (count)
  {
    writeLong(*ptr);
    count--;
    ptr++;
  }
  
  while (1)
  {
    tx_byte(0xf9);  // wait for the checksum
    mdelay(50);
    if (rx_ready()) break;
  }
  response = rx_byte();

  printf("Responded: %x\n", response);
}


int prop_init()
{
  unsigned long tmp;
  int res = ioperm(0xcf8, 8, 1);
  if (res) {
    printf("Error: IO permissions on 0xcf8\n");
    _exit(-1);
  }

  res = ioperm(0x3f8, 8, 1);
  if (res) {
    printf("Error: IO permissions on serial port\n");
    _exit(-1);
  }
  
  //  printf("IO permissions obtained\n");
  
  // Set lines as GPIO
  outl(CONTROL, 0xcf8);  // Set control register
  tmp = inl(0xcfc);
  tmp |= BIT_RST;        // clock set for GPIO function
  outl(tmp, 0xcfc);      // Set relevant lines as IO by bringing them high

  outl(DATA, 0xcf8);     // Set data register
  
  // From this point on, writes to 0xcfc set the ports, reads read back the values.
  RST_HIGH;

  //printf("Ready: %x\n", ready);
  return 0;
}








int Help()
{
  printf("\n");
  printf("BiffPROP.  Copyright (c) Bifferos.com 2010, bifferos@yahoo.co.uk\n");
  printf("\n");
  printf("Program Propeller from Bifferboard.\n");
  printf("Usage: biffprop <spin binary>\n");
  return 0;
}


void Error(char* err)
{
  printf("Error: %s\n", err);
  _exit(-1);
}


// Up to 32k in size.
unsigned char buffer[0x8001];


int Load(const char* fname)
{
  int res, fd;

  printf("Loading %s\n", fname);

  fd = open(fname,O_RDONLY);
  if (fd < 0)
  {
    printf("Unable to open file\n");
    return 0;
  }
  res = read(fd, buffer, sizeof(buffer));
  if (res==sizeof(buffer))
  {
    printf("%s too large, not a Propeller bin file?\n", fname);
    return 0;
  }
  if (res % 4)
  {
    printf("%s length should be a multiple of 4 - not a Propeller bin file?\n", fname);
    return 0;
  }
  return res;
}




int main(int argc, char** argv)
{
  int res = prop_init();
  int length;
  
  if (argc != 2)
  {
    printf("Invalid parameters\n");
    Help();
    return -1;
  }

  if (res) {
    Error("Unable to init Prop - not connected?\n");
    _exit(1);
  }

  length = Load(argv[1]);
  if (!length) {
    _exit(1);
  }

  res = prop_connect();
  printf("Prop found, version %d\n", res);

  // Upload file to Prop.
  prop_upload(buffer, length);
  
  return 0;
}


