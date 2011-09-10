#!/usr/bin/env python

"""
   Generate a 0x2000-byte Biffboot configuration block.
      
   The generated block is saved to disk, and can then be written
   to the Bifferboard with the command:
   
   dd if=config.img of=/dev/mtdblock0 bs=8k seek=2
"""

# Edit these options to give the required config block

bootsource=0    # 0=flash, 1=MMC, 2=Network
console=1       # 1=serial console enable, 0=disable
nic=1           # 0=net console disabled, 1=enabled, 2=promiscuous
boottype=1      # 1=linux, 0=flat bin, 2=multiboot/ELF, 3=coreboot payload
loadaddress=0x400000   # OK for Linux images up to 8MB, don't change!

# Add any command-line options here, e.g. console=uart,io,0x3f8
# max len is 1024 bytes.  Don't use console= cmnd-line for OpenWrt, it's
# built-in!
cmdline = "wooger=1"

# Default (0x20) allows kernel area 0x200000 bytes
# 0x10 is sensible for OpenWrt.  This must match mkimg_bifferboard.py in the
# OpenWrt sources.
kernelmax = 0x10


# No user-servicable parts beyond this line
# -----------------------------------------

import struct, sys, hashlib, os


def GetConfig():
  "Return 8K config block with correct md5"
  version = 1   # always 1 for this version.
  cfg = struct.pack("<iBBBBL", version,
    bootsource,console,nic,boottype, loadaddress)
      
  cmd = cmdline
  while len(cmd)<1024:
    cmd += "\x00"
  cfg += cmd
  
  # kernelmax is a 2-byte value
  cfg += struct.pack("<H", kernelmax)
    
  while len(cfg)<(0x2000-0x10):
    cfg += "\xff"
  
  cfg += hashlib.md5(cfg).digest()
  return cfg
  

if __name__ == "__main__":

  if not sys.argv[1:]:
    print  "usage: make_config_block.py <output>"
    sys.exit(0)

  output = sys.argv[1]

  cfg = GetConfig()
  
  if os.path.isfile(output):
    print "Refusing to clobber '%s'" % output
    sys.exit(-1)

  print "Writing config block as '%s'" % output
  file(output,"wb").write(cfg)
  

