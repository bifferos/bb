#!/usr/bin/env python

""" 
    Test program for bb_udp_upload.py.  This script pretends to be
    a bifferboard, for the purposes of testing the protocol.

"""

import socket, struct, hashlib, binascii, os, sys


# Ether protocol command-codes.
TYPE_ATTN=0        # console break-in packet.  (ack)
TYPE_RELEASE=2     # allow execution to continue. (ack)
TYPE_DATA_RESET=4  # reset data receive buffer (no ack)
TYPE_DATA_ADD=6    # send data (no ack).
TYPE_DATA_DIGEST=8   # md5 the data in buffer (md5 returned)
TYPE_DATA_WRITE=0xa  # write block to flash (ack)
TYPE_ERASE_SECTOR=0xc  # erase flash (but not config block) (ack)

ERROR_NONE  = 0x80000000
ERROR_VERIFY= 0x80000001
ERROR_FLASH = 0x80000002

def Unpack(data):
  if len(data) < 6: return None
  cmd, arg = struct.unpack("<HL", data[:6])
  return cmd, arg, data[6:]


def Reply(s, addr, cmd, arg, buf=""):
  pkt = struct.pack("<HL", cmd, arg) + buf
  #print "reply to %d with %d bytes" % (cmd-1, len(pkt))
  s.sendto(pkt, addr)
  

def Serve(fname):
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
  s.bind(("", 0xb1ff))
  print "Waiting for requests"
  
  buffer = ""
  fp = file(fname, "wb")
  
  cmd = 0xffff
  while (cmd != TYPE_RELEASE):
    pkt, addr = s.recvfrom(1000)
    cmd, arg, data = Unpack(pkt)
    rcmd = cmd + 1
    if cmd == TYPE_ATTN:
      Reply(s, addr, rcmd, arg)
    elif cmd == TYPE_RELEASE:
      Reply(s, addr, rcmd, arg)
    elif cmd == TYPE_DATA_RESET:
      buffer = ""
    elif cmd == TYPE_DATA_ADD:
      buffer += data
    elif cmd == TYPE_DATA_DIGEST:
      digest = hashlib.md5(buffer).digest()
      hex_digest = binascii.hexlify(digest)
      #print "Digest of %d bytes is %s" % (len(buffer),hex_digest)
      Reply(s, addr, rcmd, len(digest), digest)
    elif cmd == TYPE_DATA_WRITE:
      fp.write(buffer)
      Reply(s, addr, rcmd, 1, "\x00")
    elif cmd == TYPE_ERASE_SECTOR:
      #print "Erasing sector %d" % arg
      Reply(s, addr, rcmd, 0)
    else:
      #print "Invalid packet received"
      pass



if __name__ == "__main__":
  
  if not sys.argv[1:]:
    print "Need a filename to write to"
    sys.exit(1)
  
  fname = sys.argv[1]
  if os.path.exists(fname):
    print "Refusing to clobber '%s'" % fname
    sys.exit(2)
    
  Serve(fname)
  print "Server released by client, Bifferboard would normally boot now"

