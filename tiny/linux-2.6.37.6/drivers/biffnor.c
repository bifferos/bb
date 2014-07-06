// Kernel upgrade driver, by Bifferos, bifferos@yahoo.co.uk
// Only works for 8MB flash devices.


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <asm/uaccess.h>


#define BIFFBOOT_MINOR 161
#define BIFFKERNEL_MINOR 162



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
  val = *(volatile unsigned char *)(g_io_base+addr);
  
  for (i=0;i<delay;i++)
  {};
  return val;
}



static unsigned short ReadFLASH16_FAST(u32 addr)
{
  return *(volatile unsigned short *)(g_io_base+addr);
}


static u32 ReadFLASH32_FAST(u32 addr)
{
  return *(volatile u32 *)(g_io_base+addr);
}







//////////////////////////////////////////
// Flashmap functions
//////////////////////////////////////////

#define FLASHMAP_CONFIG 0x4000

// Always this value
#define FLASHMAP_CHUNK_SIZE 0x2000

#define FLASHMAP_BOOT_SIZE 0x10000

// flash detection codes.
#define FLASH_CODE_EN800 (0x7f1c225b)
#define FLASH_CODE_EN320 (0x7f1c22f9)
#define FLASH_CODE_EN640 (0x7f1c22cb)



static u32 FLASHMAP_DEVICE_SIZE = 0;
static u32 FLASHMAP_BASE = 0;
static u32 FLASHMAP_SIZE = 0;
static u32 FLASHMAP_KERNEL_SIZE = 0;
static u32 FLASHMAP_CHUNK_COUNT = 0;



static u32 flashmap_get_detected(void)
{
  return FLASH_CODE_EN640;
}



static int flashmap_init(void)
{  
  const char* detected = "Unrecognised";
  int err = 0;

  switch (flashmap_get_detected())
  {
    case FLASH_CODE_EN800:  // 1MB
      FLASHMAP_DEVICE_SIZE = 1;
      FLASHMAP_BASE = 0xfff00000; // start address.
      FLASHMAP_SIZE = 0x100000;   // size in bytes
      FLASHMAP_KERNEL_SIZE = 0xf0000;  // size of kernel to copy
      detected = "EN800";
      break;
    case FLASH_CODE_EN320:  // 4MB
      FLASHMAP_DEVICE_SIZE = 4;
      FLASHMAP_BASE = 0xffc00000;
      FLASHMAP_SIZE = 0x400000;
      FLASHMAP_KERNEL_SIZE = FLASHMAP_SIZE - 0x10000;
      detected = "EN320";
      break;
    case FLASH_CODE_EN640:  // 8MB
      FLASHMAP_DEVICE_SIZE = 8;
      FLASHMAP_BASE = 0xff800000;
      FLASHMAP_SIZE = 0x800000;
      FLASHMAP_KERNEL_SIZE = FLASHMAP_SIZE - 0x10000;
      detected = "EN640";
      break;
    default:
      err = 1;
  }
  
  pr_info("BIFFKERNEL: flash_detect() returned '%s'\n", detected);
  
  if (!err)
  {
    FLASHMAP_CHUNK_COUNT = ((FLASHMAP_SIZE-FLASHMAP_BOOT_SIZE)/FLASHMAP_CHUNK_SIZE);
  }
  
  return err;
}



// Return true if we're at the start of a sector.
static int flashmap_isboundary(unsigned long addr)
{
  if (addr & 0x1fff) return 0;   // optimise: no sector smaller than 8k
  if (addr >= FLASHMAP_SIZE) return 0;   // not one of ours.
  if (!addr) return 1;   // start of flash, always a sector

  if (addr >= 0x10000)   // bottom-boot: high sectors all 64k
  {
    // only 64k sectors allowed
    if (addr & 0xffff) return 0;  // not 64k boundary
    return 1;
  }

  // 1M devices have only 4 sectors.
  if (FLASHMAP_DEVICE_SIZE == 1)
  {
    switch (addr)
    {  // 16k:8k:8k:32k only
      case 0x4000:  case 0x6000:  case 0x8000:
        return 1;
    }
    return 0;
  }
  
  // 4,8M devices have only 8k bottom-boot blocks
  return 1;
}


// Return true if this flash sector is 0xff.
static int flashmap_iserased(unsigned long addr)
{
  // check the first:
  unsigned long val;
  unsigned long i;
  
  if (!flashmap_isboundary(addr))
    return 0;
  // do the first 
  while (addr<FLASHMAP_SIZE)  // don't go off the top!
  {
    // Check this chunk
    i = FLASHMAP_CHUNK_SIZE;
    while (i)
    {
      i-=4;
      val = ReadFLASH32_FAST(addr);
      if (val != 0xffffffff) return 0;
      addr += 4;
    }
    // We're done with this sector
    if (flashmap_isboundary(addr)) return 1;
  }
  
  // reached end of flash
  return 1;
}







//////////////////////////////////////////
//
//////////////////////////////////////////



static int EON_EraseSector(u32 addr)
{
  int prev, cur, count = 0;

  if (addr>=FLASHMAP_SIZE) 
  {
    pr_err("BIFFKERNEL: Attempt to erase sector outside allowed range");
    return -1;   // refuse to erase outside range.
  }
  
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(0xAAA,0x80);
  WriteFLASH(0xAAA,0xAA);
  WriteFLASH(0x555,0x55);
  WriteFLASH(addr,0x30);

  prev = ReadFLASH(addr,8);
  //printf("value: %x\n", prev);
  prev &= 0x40;

  cur  = ReadFLASH(addr,8);
  //printf("value: %x\n", cur);
  cur  &= 0x40;

  while (prev != cur)
  {
    prev = ReadFLASH(addr,8);
    //printf("value: %x\n", prev);
    prev &= 0x40;

    cur  = ReadFLASH(addr,8);
    //printf("value: %x\n", cur);
    cur  &= 0x40;

    if (cur & 0x20)  // DQ5==1
    {
      prev = ReadFLASH(addr,8) & 0x40;
      cur  = ReadFLASH(addr,8) & 0x40;
      if (prev!=cur) count = 0xffffff;
      break;
    }
    cur &= 0x40;
    //printf("value: %x %d\n", cur, count);
    if (count++ > 0x100000) break;   // way too long.
  }

  if (count>0x100000)
  {
    return -1;
  } else {
    return count;
  }
}




static int EON_ProgramWord(u32 addr, unsigned short val)
{
  int prev, cur;
  u32 count = 0;
  if (addr>=FLASHMAP_SIZE) 
  {
    pr_err("BIFFKERNEL: Attempt to write beyond flash range");
    return -1;  // outside flash.
  }

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
    //printf("value: %x %d\n", cur, count);
    if (count++ > 0x100000) break;   // way too long.
  }

  if (count>0x10000)
  {
    pr_err("BIFFKERNEL: Error writing to flash");
    return -1;  // error
  }

  //printf("Programmed 0x%x in %d ticks\n", addr, count);
  return count;
}


// dest will be a flash ram offset (i.e. not absolute).
// count is a count of shorts
static int EON_ProgramRange(unsigned short* src, u32 dest,u32 count)
{
  unsigned short val;
  while (count--)
  {
    val = *src;
    if (EON_ProgramWord(dest, val)<0)
    {
      pr_err("BIFFKERNEL: Word programmed incorrectly");
      return -1;   // programmed incorrectly
    }
    src++;
    dest += 2;
  }
  return 0;
}


static int EON_VerifyRange(unsigned short* src, u32 dest,u32 count)
{
  unsigned short val;
  while (count--)
  {
    val = *src;
    if (val!=ReadFLASH16_FAST(dest))
    {
      pr_err("BIFFKERNEL: Verify failed");
      return -1;   // programmed incorrectly
    }
    src++;
    dest += 2;
  }
  return 0;
}




static int EON_ReadRange(u32 src, u32* dest, u32 count)
{
  u32 val;
  count /= 4;
  while (count)
  {
    val = ReadFLASH32_FAST(src);
    *dest = val;
    dest++;
    src += 4;
    count--;
  }
  return 0;
}




// legacy  location  With config    (16k/8k/8k/32k sectors)
//    000   0x0000      117
//    001   0x2000      118
//    002   0x4000      119
//    003   0x6000      000
//    004   0x8000      001
//    ...
//    116               113
//    119               116


// Given the chunk number, return the actual location of the 8k block
static int EON_ChunkToSector(int chunk)
{
  if (chunk < (FLASHMAP_CHUNK_COUNT-3))
  {
    return chunk + 3;   // Real place to write.
  }
  else
  {
    return chunk - (FLASHMAP_CHUNK_COUNT-3);   // real place to write
  }
}


// Src = data to write
// dest = target flash offset
// len = length
static int flash_write_chunk(u16* src, u32 dest, u32 len)
{
  int tmp;
  
  // Check if the specified location makes sense.
  if ((dest + len) > (FLASHMAP_SIZE - FLASHMAP_BOOT_SIZE)) 
  {
    pr_info("BIFFKERNEL: Invalid location for flash write\n");
    return 1;
  }
  
  // check if we are on sector boundary, and erase as needed.
  if (flashmap_isboundary(dest))
  {
    // Check if this sector is all 0xff
    if (!flashmap_iserased(dest))
    {
      // Need to delete this sector first.
      //printf("Erasing sector\n");
      EON_EraseSector(dest);   
         
    }
  }
  
  tmp = EON_ProgramRange( src,
                          dest,
                          len/2);
  
  if (tmp<0) return 1;   // bail on any error.
  
  tmp = EON_VerifyRange( src,
                          dest, 
                          len/2);
  if (tmp<0) return 1;   // bail on error
  
  return 0;   // tell the caller.
}




struct kernel_fp_private {
  u32 filepos;
  u8 read_buffer[FLASHMAP_CHUNK_SIZE];  // buffers for reading/writing.
  u8 write_buffer[FLASHMAP_CHUNK_SIZE];
  u32 write_buffer_pos;
};


struct biffboot_fp_private {
  u32 filepos;
  u8 read_buffer[0x10000];  // read the entire bios
  u8 write_buffer[0x10000];
  u32 write_buffer_pos;
};




static void write_buffer_reset(struct kernel_fp_private* priv)
{
  memset(&priv->write_buffer[0], 0xff, sizeof(priv->write_buffer));
  priv->write_buffer_pos = 0;
}


// read from memory into chunk buffer
static void buffer_read(struct kernel_fp_private* priv)
{
  unsigned long flags;
  u32 chunk = priv->filepos / FLASHMAP_CHUNK_SIZE;
  u32 sect = EON_ChunkToSector(chunk) * FLASHMAP_CHUNK_SIZE;
  
  pr_info("BIFFKERNEL: Reading chunk %d\n", chunk);
  
  spin_lock_irqsave(&erase_lock, flags);
  EON_ReadRange(sect, (u32*)&priv->read_buffer[0], sizeof(priv->read_buffer));
  spin_unlock_irqrestore(&erase_lock, flags);  
}


// read from NOR into biffboot buffer.
static void biffboot_buffer_read(struct biffboot_fp_private* priv)
{
  unsigned long flags;
  
  pr_info("BIFFBOOT: Reading biffboot area\n");
  
  spin_lock_irqsave(&erase_lock, flags);
  EON_ReadRange(FLASHMAP_KERNEL_SIZE, (u32*)&priv->read_buffer[0], sizeof(priv->read_buffer));
  spin_unlock_irqrestore(&erase_lock, flags);  
}


// Write buffer to flash.  If we've just been asked to write a whole chunk, then filepos will 
// be immediately after the chunk to write, otherwise it will be inside the sector to write.
static void buffer_write(struct kernel_fp_private* priv)
{
  unsigned long flags;
  u32 chunk = priv->filepos / FLASHMAP_CHUNK_SIZE;
  u32 sect;

  if ((priv->filepos % FLASHMAP_CHUNK_SIZE) == 0)
  {
    chunk -= 1;
    pr_info("BIFFKERNEL: Writing chunk %d\n", chunk);
  }
  else
  {
    pr_info("BIFFKERNEL: Writing partial chunk %d (%d bytes)\n", chunk, priv->write_buffer_pos);
  }
    
  sect = EON_ChunkToSector(chunk) * FLASHMAP_CHUNK_SIZE;
  
  
  spin_lock_irqsave(&erase_lock, flags);
  flash_write_chunk((unsigned short*)&priv->write_buffer[0], sect, priv->write_buffer_pos);
  spin_unlock_irqrestore(&erase_lock, flags);
  
  write_buffer_reset(priv);
}


static int is_write_buffer_full(struct kernel_fp_private* priv)
{
  return priv->write_buffer_pos == sizeof(priv->write_buffer);
}


static int biffkernel_open(struct inode *inode, struct file *filp)
{
  struct kernel_fp_private* priv = kzalloc(sizeof(struct kernel_fp_private), GFP_KERNEL);
  if (!priv) return -ENOMEM;
  
  write_buffer_reset(priv);
  filp->private_data = priv;  
  return 0;
}



static ssize_t biffkernel_write(struct file *filp, const char __user *ubuf,
                               size_t count, loff_t *offp)
{
  struct kernel_fp_private* priv = filp->private_data;

  size_t remaining = count;
  size_t to_copy;
  
  while (remaining)
  {
    if (priv->filepos >= FLASHMAP_KERNEL_SIZE) 
    {
      pr_info("BIFFKERNEL: Attempt to write past end of flash kernel area (%x)\n", FLASHMAP_KERNEL_SIZE);
      if (count == remaining)   // no bytes were written and no bytes can be written.
      {
	return -ENOSPC;
      }
      break;   // attempt to write at end of file.
    }
    
    to_copy = min(sizeof(priv->write_buffer) - priv->write_buffer_pos, remaining);   // what we could write
    
    copy_from_user(&priv->write_buffer[priv->write_buffer_pos], ubuf, to_copy);
    
    priv->write_buffer_pos += to_copy; // we've added to the buffer, so reflect that.
    
    priv->filepos += to_copy;     // advance file position
    
    if (is_write_buffer_full(priv))
    {
      // write the block
      buffer_write(priv);  
    }
    
    remaining -= to_copy;         // reduce the number of bytes we've still to write in this call
    ubuf += to_copy;              // advance the user's buffer we're writing from
  }
  
  return count - remaining;
}



static ssize_t biffkernel_read(struct file *filp, char __user *ubuf,
                               size_t count, loff_t *offp)
{
  struct kernel_fp_private* priv = filp->private_data;
  size_t remaining = count;
  size_t toread;
  
  while (remaining)
  {    
    size_t readpos = priv->filepos & 0x1fff;
    
    if (priv->filepos >= FLASHMAP_KERNEL_SIZE) break;    
    
    // ensure we have data in the read buffer to read.
    buffer_read(priv);
    
    toread = min(FLASHMAP_CHUNK_SIZE - readpos, remaining);
    
    copy_to_user(ubuf, &priv->read_buffer[readpos], toread);
    
    ubuf += toread;        // update pointer to users memory
    remaining -= toread;   // update our count of bytes to copy
    priv->filepos += toread;   // update the file position.
    
  }
  
  return count - remaining;
}


loff_t biffkernel_seek(struct file *filp, loff_t offset, int whence)
{
  struct kernel_fp_private* priv = filp->private_data;
  
  loff_t newpos = priv->filepos; 
  
  switch (whence)
  {
    case SEEK_SET:
      newpos = offset;
      break;
    case SEEK_CUR:
      newpos += offset;
      break;
    case SEEK_END:
      newpos = FLASHMAP_KERNEL_SIZE;
      newpos -= offset;
      break;      
    default:
      return -EINVAL;
  }
  
  pr_info("BIFFKERNEL: Seeking to (%llx)\n", newpos);
  
  if (newpos < 0) return -EINVAL;
  
  if (newpos >= FLASHMAP_KERNEL_SIZE) 
    newpos = FLASHMAP_KERNEL_SIZE;
  
  priv->filepos = newpos;
  
  // re-read the current buffer, wherever we are now, so next read has it in the cache.
  buffer_read(priv);
  
  // Any write that was in progress will be abandoned.
  write_buffer_reset(priv);
  
  return newpos;
}


 
static int biffkernel_release(struct inode *inode, struct file *filp)
{
  struct kernel_fp_private* priv = filp->private_data;
  
  if (priv->write_buffer_pos)
  {
    // check if a tail has still to be written (<CHUNK_SIZE)
    buffer_write(priv);
  }
  
  kfree(filp->private_data);
  
  return 0;
} 
 

static struct file_operations biffkernel_fops = {
  .owner = THIS_MODULE,
  .open = biffkernel_open,
  .read = biffkernel_read,
  .write = biffkernel_write,
  .llseek = biffkernel_seek,
  .release = biffkernel_release,
};


static struct miscdevice biffkernel_device = {
  BIFFKERNEL_MINOR,
  "biffkernel",
  &biffkernel_fops,
};



////////////////////////////////////////////
// Bootloader writing code.
//



static int sanity_check(struct biffboot_fp_private* priv)
{
  // Is it Biffboot?
  if (strncmp(&priv->write_buffer[0xff00], "Biffboot", 8)!=0) return 0;
  // Does it have a JMP at the entry-point?
  if (priv->write_buffer[0xfff0] != 0xe9) return 0;
  if (priv->write_buffer[0xfff1] != 0x0a) return 0;
  if (priv->write_buffer[0xfff2] != 0xf1) return 0;
  // Has the last byte got set correctly?
  if (priv->write_buffer[0xffff] != 0xff) return 0;
  return 1;
}


static void WriteSector(struct biffboot_fp_private* priv)
{
  u32 count = 0;
  u16* ptr = (u16*)&priv->write_buffer[0];
  while (count < priv->write_buffer_pos)
  {
    EON_ProgramWord(count + FLASHMAP_KERNEL_SIZE, *ptr);
    ptr += 1;
    count += 2;
  }
}



static int biffboot_open(struct inode *inode, struct file *filp)
{
  struct biffboot_fp_private* priv = kzalloc(sizeof(struct biffboot_fp_private), GFP_KERNEL);
  if (!priv) return -ENOMEM;
  filp->private_data = priv;
  return 0;
}



static ssize_t biffboot_read(struct file *filp, char __user *ubuf,
                               size_t count, loff_t *offp)
{
  struct biffboot_fp_private* priv = filp->private_data;
  size_t remaining = count;
  size_t toread;
  
  while (remaining)
  {    
    if (priv->filepos >= 0x10000) break;
    
    // ensure we have data in the read buffer to read.
    biffboot_buffer_read(priv);
    
    toread = min(0x10000 - priv->filepos, remaining);
    
    copy_to_user(ubuf, &priv->read_buffer[priv->filepos], toread);
    
    ubuf += toread;        // update pointer to users memory
    remaining -= toread;   // update our count of bytes to copy
    priv->filepos += toread;   // update the file position.
    
  }
  
  return count - remaining;
}


static ssize_t biffboot_write(struct file *filp, const char __user *ubuf,
                               size_t count, loff_t *offp)
{
  unsigned long flags;
  
  struct biffboot_fp_private* priv = filp->private_data;
  
  if ((priv->write_buffer_pos + count) > 0x10000) return -EINVAL;
  
  copy_from_user(&priv->write_buffer[priv->write_buffer_pos], ubuf, count);
  priv->write_buffer_pos += count;
  
  if (priv->write_buffer_pos == 0x10000)
  {
    pr_info("BIFFBOOT: Write buffer is full, writing to flash now\n");

    if (!sanity_check(priv)) 
    {
      pr_info("BIFFBOOT: Doesn't look like Biffboot, refusing to flash it, sorry.\n");
      return -EINVAL;
    }
    
    pr_info("BIFFBOOT: Sanity check passed, erasing BIOS area\n");
    spin_lock_irqsave(&erase_lock, flags);
    EON_EraseSector(FLASHMAP_KERNEL_SIZE);
    spin_unlock_irqrestore(&erase_lock, flags);
    
    pr_info("BIFFBOOT: Writing data, do not switch off until complete\n");
    WriteSector(priv);
    pr_info("BIFFBOOT: BIOS area written.  Please reboot.\n");
  }
  return count;
}
  
 
static int biffboot_release(struct inode *inode, struct file *filp)
{
  kfree(filp->private_data);  
  return 0;
}  
 

static struct file_operations biffboot_fops = {
  .owner = THIS_MODULE,
  .open = biffboot_open,
  .read = biffboot_read,
  .write = biffboot_write,
  .release = biffboot_release,
};


static struct miscdevice biffboot_device = {
  BIFFBOOT_MINOR,
  "biffboot",
  &biffboot_fops,
};




static int __init biffnor_init(void)
{
  g_io_base = (int)ioremap(0xff800000, 0x800000);
  pr_info("BIFFNOR: mapped flash to 0x%x.\n", g_io_base);
  
  // Register misc device
  if (misc_register(&biffkernel_device)) {
    pr_info("BIFFNOR: Couldn't register kernel device %d\n", BIFFKERNEL_MINOR);
    return -EBUSY;
  }
  
  if (misc_register(&biffboot_device)) {
    pr_info("BIFFNOR: Couldn't register biffboot device %d\n", BIFFKERNEL_MINOR);
    misc_deregister(&biffkernel_device);
    return -EBUSY;
  }
  
  flashmap_init();
  
  pr_info("BIFFNOR: bifferboard flash driver (v1.0) by bifferos, loaded.\n");
  return 0;
}


static void __exit biffnor_exit(void)
{
  misc_deregister(&biffkernel_device);
  misc_deregister(&biffboot_device);
  pr_info("BIFFNOR: bifferboard flash driver (v1.0) by bifferos, unloaded.\n");
}


module_init(biffnor_init);
module_exit(biffnor_exit);


