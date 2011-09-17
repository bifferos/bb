#!/usr/bin/env python

"""
   Create firmware for 4/8MB Bifferboards, suitable for uploading using
   either bb_upload8.py or bb_eth_upload8.py
"""

import struct, sys, os


# No need to change this for 4MB devices, it's only used to tell you if 
# the firmware is too large!
flash_size = 0x800000

# This is always the same, for 1MB, 4MB and 8MB devices
config_extent = 0x6000


if __name__ == "__main__":

  if len(sys.argv) != 5:
    print  "usage: mkimg_bifferboard.py <kernel> <rootfs> <output file> <kmax>"
    sys.exit(-1)
    
  bzimage = sys.argv[1]
  rootfs = sys.argv[2]
  target = sys.argv[3]  
  kmax = eval(sys.argv[4])
  
  kernel_extent = kmax * 0x10000

  # Kernel first
  fw = file(bzimage).read()
  kernel_size = len(fw)
  kernel_max = kernel_extent - config_extent
  if kernel_size > kernel_max:
    print "Kernel size:", kernel_size
    print "Kmax setting: 0x%x" % kmax
    print "Max kernel allowed:", kernel_max
    raise IOError("Kernel too large")

  # Pad up to end of kernel partition
  while len(fw) < (kernel_extent - config_extent):
    fw += "\xff"

  fw += file(rootfs).read()

  # Check length of total
  if len(fw) > (flash_size - 0x10000 - config_extent):
    raise IOError("Rootfs too large")

  file(target,"wb").write(fw)
  print "Firmware written to '%s'" % target
