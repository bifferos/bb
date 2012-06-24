#!/usr/bin/env python
"""
    Copyright (c) Bifferos 2009 (sales@bifferos.com)
    
    This is free software, licensed under the GNU General Public License v2.
    See COPYING for more information.
"""

import rdc, os, sys, struct

if __name__ == "__main__":

  r = rdc.RDC()

  fname = sys.argv[1]
  fp = file(fname)
  flen = os.stat(fname).st_size
  count = 0
  print "Sector erase"
  r.EonSectorErase(0xffff0000)
  print "Programming"
  while count<flen:
    dat = struct.unpack("<H", fp.read(2))[0]
    addr = 0xffff0000 + count
    print "0x%08x <-- 0x%04x" % (addr, dat)
    if dat != 0xffff:
      r.EonProgram(addr, dat)
    count += 2
    

  data = r.DumpMem(0xffff0000, 0x80)
  print repr(data)


