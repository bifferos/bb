
#ifdef BIFFJTAG_STANDALONE    // i.e. not part of busybox

#include <stdio.h>
#include <unistd.h>
#include <string.h>


#else

#include "libbb.h"

#endif

#include <sys/io.h>
#include "jtag.h"
#include "rdc.h"











#define CONTROL 0x80003848
#define DATA 0x8000384c

#define BIT_TMS  (1<<11)   // 11
#define BIT_TDI  (1<<13)   // 13
#define BIT_TDO  (1<<9)    // 9
#define BIT_TCK  (1<<12)   // 12



int rdc_init(void)
{
  unsigned long ready=0;
  unsigned long tmp;
  int res = ioperm(0xcf8, 8, 1);
  if (res) {
    printf("Error: IO permissions\n");
    _exit(-1);
  }
  
//  printf("IO permissions obtained\n");
  
  // Set lines as GPIO
  outl(CONTROL, 0xcf8);    // Set control register
  tmp = inl(0xcfc);
  tmp |= BIT_TDI|BIT_TCK|BIT_TMS|BIT_TDO;  // ensure these are GPIO function
  outl(tmp, 0xcfc);  // Set relevant lines as IO by bringing them high

  outl(DATA, 0xcf8);    // Set data register
  
  // From this point on, writes to 0xcfc set the ports, reads read back the values.
  outl(BIT_TDO, 0xcfc);    // TDO as input (high)

  // Call assembler init routine... waits forever.
  ready = jtag_init();
  if (ready != 0x8381)
  {
    printf("Error initialising: 0x%x != 0x8381\n", (unsigned)ready);
    return -1;
  }
  //printf("Ready: %x\n", ready);
  return 0;
}



unsigned long rdc_Detect(void)
{
  int ret = 0;
  jtag_WriteMem16(0xffffaaaa, 0xaaaa);
  jtag_WriteMem16(0xffff5554, 0x5555);
  jtag_WriteMem16(0xffffaaaa, 0x9090);

  ret = jtag_ReadMem16(0xffff0000);
  ret <<= 8;
  ret |= jtag_ReadMem16(0xffff0200);
  ret <<= 16;
  ret |= jtag_ReadMem16(0xffff0002);
  
  jtag_WriteMem16(0xffff0000, 0xf0f0);
  return ret;
}


/*
unsigned long rdc_Detect()
{
  int ret = 0;
  jtag_WriteMem16(0xffffaaaa, 0xaaaa);
  jtag_WriteMem16(0xffff5554, 0x5555);
  jtag_WriteMem16(0xffffaaaa, 0x9090);

  ret = jtag_ReadMem16(0xffff0000);
  printf("0xffff0000: %x\n", ret);
  //ret <<= 8;
  ret = jtag_ReadMem16(0xffff0200);
  printf("0xffff0200: %x\n", ret);
  //ret <<= 16;
  ret = jtag_ReadMem16(0xffff0002);
  printf("0xffff0002: %x\n", ret);
  
  jtag_WriteMem16(0xffff0000, 0xf0f0);
  return ret;
}
*/


void rdc_EonSectorErase(unsigned long addr)
{
  unsigned long r=1;
  jtag_WriteMem16(0xffffaaaa, 0xaaaa);
  jtag_WriteMem16(0xffff5554, 0x5555);
  jtag_WriteMem16(0xffffaaaa, 0x8080);
  jtag_WriteMem16(0xffffaaaa, 0xaaaa);
  jtag_WriteMem16(0xffff5554, 0x5555);
  jtag_WriteMem16(addr, 0x3030);
  while (0xffff != r)
  {
    r = jtag_ReadMem16(addr);
  }
}



int rdc_EonProgram(unsigned long addr, unsigned short value)
{
  int i=0;
    jtag_WriteMem16(0xffffaaaa, 0xaaaa);
    jtag_WriteMem16(0xffff5554, 0x5555);
    jtag_WriteMem16(0xffffaaaa, 0xA0A0);
    jtag_WriteMem16(addr, value);
    while (value != jtag_ReadMem16(addr))
    {
      i++;
      if (i>10000000) return -1;
    }
    return 0;    
}


// Read 16-bit status value
static unsigned short Status(void)
{
  unsigned short tmp;
  jtag_write_ir(0x0f);
  jtag_idle_to_shift_dr();
  tmp = jtag_read_dr16() & 0xffff;
  jtag_update_to_idle();   // last one
  return tmp;
}


// Read 32-bit value
static void ReadData32(unsigned char command, unsigned long* data, size_t data_length)
{
    jtag_write_ir(command);
    if (data_length)
    {
      jtag_idle_to_shift_dr();
      data_length /= 4;
      while (data_length)
      {
        *data = jtag_read_dr32();
        data++;
          
        if (data_length==1)
        {
          jtag_update_to_idle();   // last one
        } else {
          jtag_update_to_shift_dr();   // more to go
        }
        data_length--;
      }
    }
}


static void WriteData(unsigned char command, unsigned char* data, size_t data_length)
{
  unsigned long* ptr = (unsigned long*)data;
  jtag_write_ir(command);
  if (data_length)
  {
    jtag_idle_to_shift_dr();
    data_length /= 4;
    while (data_length)
    {
      jtag_write_dr32(*ptr);
      if (data_length==1) {
        jtag_update_to_idle();   // last one
      } else {
        jtag_update_to_shift_dr();  // more to go
      }
      data_length--;
      ptr++;
    }
  }
}


static void ResetFlags(void)
{
  struct _bits {
    unsigned long val1;
    unsigned long val2;
    unsigned long val3;
  } tosend;
  tosend.val1 = 0x2;
  tosend.val2 = 0xff00;
  tosend.val3 = 0x9090f000;
  WriteData(0x0d, (unsigned char*)&tosend, sizeof(tosend) );
}

/*
static void ReadFlags(void)
{
  struct _bits {
    unsigned long val1;
    unsigned long val2;
    unsigned long val3;
  } tosend;
  
  tosend.val1 = 0;
  tosend.val2 = 0;
  tosend.val3 = 0;
  
  ReadData32(0x0c, &tosend.val1, sizeof(tosend) );

  printf("val1 %x\n", (unsigned int)tosend.val1);
  printf("val2 %x\n", (unsigned int)tosend.val2);
  printf("val3 %x\n", (unsigned int)tosend.val3);
}
*/


static void DumpMem(unsigned long addr, unsigned char* buffer, unsigned long count)
{
  unsigned int i;
  unsigned char* ptr = buffer;
  unsigned char tmp;
  
  if (count>0x80)
  {
    printf("Memory can only be dumped in 0x80 blocks\n");
    return;
  }
  ResetFlags();
  
  jtag_MemAccess(addr, 0x18);
  
  for (i=0;i<count;i++)
  {
    jtag_update_to_shift_dr();  // more to go
    tmp = (jtag_read_dr8() & 0xff);
    *ptr = tmp;
    ptr++;
  }
  jtag_update_to_idle();
}


static int RunCode(unsigned char* data, int count)
{
    unsigned short s1;
    ResetFlags();
    WriteData(0x06, data, count);      // load
    WriteData(0x02, data, 0);             // execute

    s1 = Status();
    if (s1 == 0x8381)
    {
      return 0;
    }
    printf("Code error %x\n", s1);
    return -1;   // failed to execute code.
}


unsigned long rdc_eax(void)
{
  unsigned long ret=0;
  ReadData32(0x0e, &ret, 4);
  return ret;
}


//  unsigned long eax = rdc_eax();
//  printf("eax before : %x\n", eax);
//  ret = RunCode((unsigned char*)"\xb8\x42\x00\x90", 4);
//  eax = rdc_eax();
//  printf("eax: %x\n", eax);


// the contents of the two defines below is generated by rdc_assemble.py:
//  ./rdc_assemble.py bus_control.S bus_control.bin
//  bus_control.h: bus_control.bin
//	xxd -i bus_control.bin > bus_control.h
// The binary consists of a byte length field (in practise
//#include "bus_control.h"


static unsigned char bus_control_bin[] = {
  0x04, 0xba, 0xf8, 0x0c, 0x90, 0x08, 0x66, 0xb8, 0x40, 0x38, 0x00, 0x80,
  0x90, 0x90, 0x04, 0x66, 0xef, 0x90, 0x90, 0x04, 0xba, 0xfc, 0x0c, 0x90,
  0x08, 0x66, 0xb8, 0x00, 0x06, 0xff, 0x87, 0x90, 0x90, 0x04, 0x66, 0xef,
  0x90, 0x90
};
static unsigned int bus_control_bin_len = 38;



int rdc_bus_control(void)
{
  unsigned char length;
  unsigned char* ptr = bus_control_bin;
  unsigned char* end_ptr = bus_control_bin + bus_control_bin_len;
  int ret = 0;
  while (ptr < end_ptr)
  {
    length = *ptr;
    ptr++;
    ret = RunCode(ptr, length);
    if (ret)
    {
      return ret;
    }
    ptr += length;
  }
  return 0;
}




int rdc_Dump80(unsigned long address, unsigned char *buffer)
{
  ResetFlags();
  DumpMem(address,buffer, 0x80);
  return 0;
}


int rdc_Dump2000(unsigned long address, unsigned char *buffer)
{
//  printf("Dumping %lx\n", address);
  unsigned char data[0x100];
  int i;
  for (i=0;i<0x2000;i+=0x80)
  {
    ResetFlags();
    DumpMem(address+i,data, 0x80);
    // write to file.
    memcpy(buffer, data, 0x80);
    buffer += 0x80;
  }
  printf("Done\n");
  return 0;
}



int rdc_DumpSector(unsigned long address, unsigned char *buffer)
{
//  printf("Dumping %lx\n", address);
  unsigned char data[0x100];
  int i;
  for (i=0;i<0x10000;i+=0x80)
  {
    ResetFlags();
    DumpMem(address+i,data, 0x80);
    // write to file.
    memcpy(buffer, data, 0x80);
    buffer += 0x80;
  }
  return 0;
}

/*
    case F_WRITE_DATA:
      WriteData(pkt->rdc_cmd, &pkt->data, pkt->data_length);
      pkt->result = 0;  // can't check status
      break;
    case F_WRITE_IR:
      jtag_write_ir(pkt->rdc_cmd);
      pkt->result = 0;  // can't check status
      break;
*/

