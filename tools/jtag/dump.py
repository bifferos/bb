#!/usr/bin/env python
"""
    Copyright (c) Bifferos 2009 (sales@bifferos.com)
    
    This is free software, licensed under the GNU General Public License v2.
    See COPYING for more information.
"""


import rdc, os

if __name__ == "__main__":

  r = rdc.RDC()

  data = r.DumpSector(0xffff0000)
  fp = file("firmware.bin","wb")
  fp.write(data)
  fp.close()

  os.system("hexdump -Cv firmware.bin | less")

