
//usage:#define biffjtag_trivial_usage
//usage:       "[program|dump] FILE"
//usage:#define biffjtag_full_usage "\n\n"
//usage:       "Program or dump bootloader from Bifferboard\n"
//usage:#define biffjtag_example_usage
//usage:       "# biffjtag program biffboot.bin\n"



#ifdef BIFFJTAG_STANDALONE    // i.e. not part of busybox

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

#else

#include "libbb.h"

#endif

#include "rdc.h"







static int Help(void)
{
  printf("\n");
  printf("BiffJTAG.  Copyright (c) Bifferos.com 2021, bifferos@yahoo.co.uk\n");
  printf("\n");
  printf("Program to or dump from Bifferboard bootloader flash.\n");
  printf("Usage: biffjtag program SOURCE_FILE\n");
  printf("  or:  biffjtag dump DEST_FILE (dump bootloader)\n");
  printf("  or:  biffjtag dumpconfig DEST_FILE (config block)\n");
  printf("  or:  biffjtag mac DEST_FILE (mac address)\n");
  return 0;
}


static void Error(const char* err)
{
  printf("Error: %s\n", err);
  _exit(-1);
}



static unsigned char buffer[0x10000];



// Dump the sector out to 'out.bin'
static void DumpBIOS(const char* fname)
{  
  int fd, res;
  // Dump out the BIOS area
  printf("Dumping\n");
  rdc_DumpSector(0xffff0000, buffer);
   
  fd = open(fname,O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);
  res = write(fd, buffer, sizeof(buffer));
  printf("Written %d bytes to '%s'\n", res, fname);
}


// Dump the all out to 'out.bin'
static void DumpAll(unsigned long addr, const char* fname)
{
  unsigned int count = 0;
  int max;
  int fd, res;

  fd = open(fname, O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);
  
  max = (0xffffff00 - addr)/0x10000 + 1;

  printf("Dumping to '%s'\n", fname);
  while (count<max)
  {
    rdc_DumpSector(addr+(count*0x10000), buffer);   
    res = write(fd, buffer, sizeof(buffer));
    count += 1;
    printf("Written %d/%d sectors to '%s'\n", count, max, fname);
  }
}


static void PrintMAC(unsigned char* mac)
{
  int i;
  for (i=0;i<6;i++)
  {
    printf("%02x", mac[i]);
    if (i!=5)
      printf(":");
  }
}

// Dump just the MAC address part, base is the base address of flash
static void DumpMAC(unsigned long base)
{  
  int fd, res;
  base += 0x4000;  // move to the config
  base += 1036;
  rdc_Dump80(base, buffer);

  fd = open("mac.bin", O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);
  buffer[6] = 0;
  buffer[7] = 0;
  res = write(fd, buffer, 8);
  if (res != 8)
  {
    printf("Unable to write all bytes to out.bin\n");
    _exit(1);
  }
  
  printf("Detected MAC: ");
  PrintMAC(buffer);
  printf("\n");
}



static void DumpConfig(unsigned long base)
{  
  int fd, res;
  base += 0x4000;  // move to the config
  rdc_Dump2000(base, buffer);

  fd = open("config.bin", O_CREAT|O_RDWR, S_IRWXU|S_IRGRP|S_IROTH);
  res = write(fd, buffer, 0x2000);
  if (res != 0x2000)
  {
    printf("Unable to write all bytes to config.bin\n");
    _exit(1);
  }
  
  printf("Config written to 'config.bin'\n");
}



static void ProgramConfig(unsigned long base, const char* fname)
{
  int res, fd;
  unsigned long addr = base;
  unsigned long off;
  unsigned short val;

  printf("Programming config from %s\n", fname);

  fd = open(fname,O_RDONLY);
  res = read(fd, buffer, sizeof(buffer));
  if (res!=0x2000)
  {
    printf("config file '%s' is incorrect size (should be 0x2000)\n", fname);
    return;
  }

  rdc_EonSectorErase(base);

  for (off=0;off<0x2000;off += 2)
  {
    val = *((unsigned short*)&buffer[off]);
    if (val==0xffff) continue;
    res = rdc_EonProgram(addr+off, val);
    if (res)
    {
      printf("Error programming at: %lx\n", addr+off);
      return;
    }
  }
}



static void EraseBootloader(void)
{
  printf("Erasing\n");
  rdc_EonSectorErase(0xffff0000);
}



static void ProgramBootloader(const char* fname)
{
  int res, fd;
  unsigned long addr = 0xffff0000;
  unsigned long off;
  unsigned short val;

  printf("Programming %s\n", fname);

  fd = open(fname,O_RDONLY);
  res = read(fd, buffer, sizeof(buffer));
  if (res!=sizeof(buffer))
  {
    printf("Error reading firmware file\n");
    return;
  }

  for (off=0;off<0xffff;off += 2)
  {
    val = *((unsigned short*)&buffer[off]);
    if (val==0xffff) continue;
    res = rdc_EonProgram(addr+off, val);
    if (res)
    {
      printf("Error programming at: %lx\n", addr+off);
      return;
    }
  }
}



static unsigned int Manufacturer(void)
{
  unsigned int ret = rdc_Detect();
  const char* f;
  unsigned long base = 0;
  switch (ret)
  {
    case 0x7f1c225b:
      f = "EN29LV800B";
      base = 0xfff00000;
      break;
    case 0x7f1c22f9:
      f = "EN29LV320B";
      base = 0xffc00000;
      break;
    case 0x7f1c22cb:
      f = "EN29LV640B";
      base = 0xff800000;
      break;
    case 0xc2c222cb:
      f = "MX29LV640B";
      base = 0xff800000;
      break;
    default:
      printf("Unknown flash: %x\n", ret);
      f = "<UNKNOWN>";
  }
  printf("Detected: %s\n", f);
  return base;
}


#ifdef BIFFJTAG_STANDALONE
int main(int argc, char** argv)
#else
int biffjtag_main(int argc, char** argv) MAIN_EXTERNALLY_VISIBLE;
int biffjtag_main(int argc, char** argv)
#endif
{
  unsigned long base;
  int res;

  if (argc != 3)
  {
    printf("Invalid parameters\n");
    Help();
    return -1;
  }

  res = rdc_init();
  if (res) {
    Error("Unable to init JTAG\n");
    _exit(1);
  }

  res = rdc_bus_control();
  if (res) {
    printf("Unable to run code (%x)\n", res);
    _exit(1);
  }
  
  base = Manufacturer();
  
  if (!base)
  {
    _exit(1);
  }
    
  if (strcmp(argv[1], "dump")==0) {
    DumpBIOS(argv[2]);
  } else if (strcmp(argv[1], "erase")==0) {
    EraseBootloader();
  } else if (strcmp(argv[1], "program")==0) {
    EraseBootloader();
    ProgramBootloader(argv[2]);
  } else if (strcmp(argv[1], "mac")==0) {
    DumpMAC(base);
  } else if (strcmp(argv[1], "dumpconfig")==0) {
    DumpConfig(base);
  } else if (strcmp(argv[1], "programconfig")==0) {
    ProgramConfig(base,"config.bin");
  } else if (strcmp(argv[1], "dumpall")==0) {
    DumpAll(base, argv[2]);
  } else {
    return Help();
  }
  
  return 0;
}


