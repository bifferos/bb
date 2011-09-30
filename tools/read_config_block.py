#!/usr/bin/env python

"""
   Display a 0x2000-byte Biffboot configuration block in human-readable form.
      
"""

import struct, sys, hashlib

config_length = 0x2000

config_keys = ('version', 'bootsource', 'console', 'nic', 'boottype', 'loadaddress', 'cmdline', 'kernelmax')

optionvalues = {
  "bootsource": {"flash": 0, "MMC": 1, "Net": 2},
  "console": {"disabled": 0, "enabled": 1},
  "nic": {"disabled": 0, "enabled": 1, "promiscuous": 2},
  "boottype": {"flatbin": 0, "Linux": 1, "Multiboot": 2, "Coreboot": 3}
}

def GetValues(config):
  values = dict(zip(config_keys, struct.unpack("<iBBBBL1024sH", config[:struct.calcsize("<iBBBBL1024sH")])))
  strend = 1023
  while (values['cmdline'][strend] == "\x00"):
    strend -= 1
  values['cmdline'] = values['cmdline'][:strend+1]
  return values
  

if __name__ == "__main__":

  if len(sys.argv) != 2 and len(sys.argv) != 3:
    print  "usage: read_config_block.py <config block> [config key]"
    sys.exit(-1)

  config = open(sys.argv[1]).read()

  if len(config) != config_length:
    print "Config block has wrong size (must be 8KB)"
    sys.exit(-1)

  if hashlib.md5(config[:config_length-0x10]).digest() != config[config_length-0x10:]:
    print "Config block is corrupt (wrong MD5)"
    sys.exit(-1)

  optionvalues_inverse = dict()
  for key in optionvalues:
    optionvalues_inverse[key] = dict()
    for text, number in optionvalues[key].items():
       optionvalues_inverse[key][number] = text

  print_key = None
  if len(sys.argv) == 3:
    if sys.argv[2] not in config_keys:
      print "Invalid config key"
      sys.exit(-1)
    print_key = sys.argv[2]

  configvalues = GetValues(config)

  for key in config_keys:
    if key == 'version': # no point printing this
       continue
    if not print_key:
      print key + ":",
    if not print_key or key == print_key:
      if key in optionvalues_inverse:
        print optionvalues_inverse[key][configvalues[key]]
      elif key == 'loadaddress' or key == 'kernelmax':
        print "0x%x" % configvalues[key]
      else:
        print configvalues[key]

