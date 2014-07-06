// BIOS upgrade driver, by Bifferos, bifferos@yahoo.co.uk


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include "biff.h"


MODULE_AUTHOR("bifferos");
MODULE_LICENSE("GPL");


static unsigned g_io_base=0;


static DEFINE_SPINLOCK(erase_lock);


static void WriteFLASH(unsigned long addr, unsigned char data)
{
  *(volatile unsigned char *)(g_io_base+addr) = data;
}
static void WriteFLASH16(unsigned long addr, unsigned short data)
{
  *(volatile unsigned short *)(g_io_base+addr) = data;
}


static unsigned char ReadFLASH(unsigned long addr, int delay)
{
  unsigned char val;
  int i;
  if (addr>0x10000) {
    pr_info("BIFFUPGRADE: Error reading flash, address too large\n");
    return 0;
  }
  val = *(volatile unsigned char *)(g_io_base+addr);
  
  for (i=0;i<delay;i++)
  {};
  return val;
}


static int EraseSector(void)
{
  int prev, cur, count = 0;
  
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0xAAA,0x80);
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0,0x30);   // offset

  prev = ReadFLASH(0,8);
  prev &= 0x40;

  cur = ReadFLASH(0,8);
  cur  &= 0x40;

  while (prev != cur)
  {
    prev = ReadFLASH(0,8);
    prev &= 0x40;

    cur  = ReadFLASH(0,8);
    cur  &= 0x40;

    if (cur & 0x20)  // DQ5==1
    {
      prev = ReadFLASH(0,8) & 0x40;
      cur  = ReadFLASH(0,8) & 0x40;
      if (prev!=cur) count = 0xffffff;
      break;
    }
    cur &= 0x40;
    if (count++ > 0x100000) break;   // taken too long
  }

  if (count>0x100000)
  {
    pr_info("BIFFUPGRADE: Timeout erasing sector\n");
    return -1;   // timeout?
  } else {
    pr_info("BIFFUPGRADE: Sector erased in %d ticks\n", count);
    return count;
  }
}


static int ProgramWord(unsigned long addr, unsigned short val)
{
  int prev, cur;
  unsigned long count = 0;

  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0xAAA,0xA0);
  WriteFLASH16(addr,val);
  prev = ReadFLASH(addr,1) & 0x40;
  cur  = ReadFLASH(addr,1) & 0x40;
  while (prev != cur)
  {
    prev = ReadFLASH(addr,1) & 0x40;
    cur  = ReadFLASH(addr,1);
    if (cur & 0x20)  // DQ5==1
    {
      prev = ReadFLASH(addr,1) & 0x40;
      cur  = ReadFLASH(addr,1) & 0x40;
      if (prev!=cur) count = 0xffffff;
      break;
    }
    cur &= 0x40;
    if (count++ > 0x100000) break;   // way too long.
  }

  if (count>0x10000)
    return -1;  // error

  return count;
}




static u32 g_writepos = 0;
static u8 g_buffer[0x10000];


static int sanity_check(void)
{
  // Is it Biffboot?
  if (strncmp(&g_buffer[0xff00], "Biffboot", 8)!=0) return 0;
  // Does it have a JMP at the entry-point?
  if (g_buffer[0xfff0] != 0xe9) return 0;
  if (g_buffer[0xfff1] != 0x0a) return 0;
  if (g_buffer[0xfff2] != 0xf1) return 0;
  // Has the last byte got set correctly?
  if (g_buffer[0xffff] != 0xff) return 0;
  return 1;
}


static void WriteSector(void)
{
  u32 count = 0;
  u16* ptr = (u16*)&g_buffer[0];
  while (count < g_writepos)
  {
    ProgramWord(count, *ptr);
    ptr += 1;
    count += 2;
  }
}



static int biffupgrade_open(struct inode *inode, struct file *filp)
{
  g_writepos = 0;
  memset(&g_buffer[0], 0, sizeof(g_buffer));
  return 0;
}


static ssize_t biffupgrade_write(struct file *filp, const char __user *ubuf,
                               size_t count, loff_t *offp)
{
  unsigned long flags;
  
  if ((g_writepos + count) > 0x10000) return -EINVAL;
  
  copy_from_user(&g_buffer[g_writepos], ubuf, count);
  g_writepos += count;
  
  if (g_writepos == 0x10000)
  {
    pr_info("BIFFUPGRADE: Write buffer is full, writing to flash now\n");

    if (!sanity_check()) 
    {
      pr_info("BIFFUPGRADE: Doesn't look like Biffboot, refusing to flash it, sorry.\n");
      return -EINVAL;
    }
    
    pr_info("BIFFUPGRADE: Sanity check passed, erasing BIOS area\n");
    spin_lock_irqsave(&erase_lock, flags);
    EraseSector();
    spin_unlock_irqrestore(&erase_lock, flags);
    
    pr_info("BIFFUPGRADE: Writing data, do not switch off until complete\n");
    WriteSector();
    pr_info("BIFFUPGRADE: BIOS area written.  Please reboot.\n");
  }
  return count;
}
  
 

static struct file_operations biffupgrade_fops = {
  .owner = THIS_MODULE,
  .open = biffupgrade_open,
  .write = biffupgrade_write,
};


static struct miscdevice biffupgrade_device = {
  BIFFUPGRADE_MINOR,
  "biffupgrade",
  &biffupgrade_fops,
};







static int __init biffupgrade_init(void)
{
  // Register misc device
  if (misc_register(&biffupgrade_device)) {
    pr_info("BIFFUPGRADE: Couldn't register device %d\n", BIFFUPGRADE_MINOR);
    return -EBUSY;
  }
  
  g_io_base = (int)ioremap(0xffff0000, 0x10000);
  
  pr_info("BIFFUPGRADE: mapped BIOS to 0x%x.\n", g_io_base);	
  pr_info("BIFFUPGRADE: biffupgrade driver (v1.0) by bifferos, loaded.\n");
  pr_info("BIFFUPGRADE: >>>>>>WARNING<<<<<< This can trash your bootloader, use with caution.\n");
  return 0;
}


static void __exit biffupgrade_exit(void)
{
  misc_deregister(&biffupgrade_device);
  pr_info("BIFFUPGRADE: biffupgrade driver (v1.0) by bifferos, unloaded.\n");
}


module_init(biffupgrade_init);
module_exit(biffupgrade_exit);


