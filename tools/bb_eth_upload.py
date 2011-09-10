#!/usr/bin/env python

"""
   To be used with Biffboot v2.0 (Bifferboards ordered after 9 May 2009).
"""

import socket,struct,time,os,sys, hashlib


def GetMac(dotted):
  " '00:00:01:02:03:04' --> '\x00\x00\x01\x02\x03\x04' " 
  return "".join([chr(eval("0x"+i)) for i in dotted.split(":")])

def GetMyMac(iface):
  "Get the MAC address of the given interface."
  dotted = file("/sys/class/net/%s/address" % iface).read()
  return GetMac(dotted)


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


g_sectors = [
  0x00000000,
#  0x00004000, # comment this in to erase the cfg sector (not recommended)
  0x00006000,
  0x00008000,
  0x00010000,
  0x00020000,
  0x00030000,
  0x00040000,
  0x00050000,
  0x00060000,
  0x00070000,
  0x00080000,
  0x00090000,
  0x000A0000,
  0x000B0000,
  0x000C0000,
  0x000D0000,
  0x000E0000
]


# How long in seconds we wait for a response to the various commands 
g_timeout = {
TYPE_ATTN:0.001,
TYPE_RELEASE:0.05, 
TYPE_DATA_WRITE:1.00, 
TYPE_DATA_DIGEST:0.05,
TYPE_ERASE_SECTOR:5.0
}



class BiffSocket:

  def __init__(self, iface, dest, proto):
    soc = socket.socket(socket.PF_PACKET, socket.SOCK_RAW)
    soc.bind((iface,proto))
    self.src = GetMyMac(iface)
    soc.setblocking(0)
    self.soc = soc
    self.dest = dest
    self.proto = proto

  def Receive(self, cmd):
    data = "----"
    while data:
      try:
        data = self.soc.recv(60)
        dst = data[:6]
        if dst == "\xff\xff\xff\xff\xff\xff": continue
        src = data[6:12]
        if src != self.dest: continue
        proto = data[12:14]
        if struct.unpack(">H",proto)[0] != 0xb1ff: continue
        type_s = data[14:16]
        arg_s = data[16:20]
        arg = struct.unpack("<L", arg_s)[0]
        buffer = data[20:]
        val = struct.unpack("<H", type_s)[0];
        if val != (cmd | 1): continue
      
        # the response we're looking for
        return (arg, buffer)
      except socket.error:
        data = ""
    return (None,None)


  def Send(self,cmd,arg, data=""):
    cmd_p = struct.pack("<H",cmd)
    arg_p = struct.pack("<L",arg)
    header = self.dest + self.src + struct.pack(">H",self.proto)
    self.soc.send(header + cmd_p + arg_p + data)

    if cmd in g_timeout.keys():
      count = g_timeout[cmd]
      while count>0:
        time.sleep(0.001)   # wait for response
        count -= 0.001
        arg, buffer = self.Receive(cmd)
        if arg!=None:
          return (True, buffer[:arg])

    return (False, "")



def DataToChunks(data, ch_size):
  "Split data into chunks"
  out = []
  while data:
    chunk = data[:ch_size]
    while len(chunk)<ch_size:
      chunk += "\xff"
    out.append(chunk)
    data = data[ch_size:]
  return out


def Transfer(soc, chunk, offset):
  soc.Send(TYPE_DATA_RESET, 0)
  for i in DataToChunks(chunk, 512):
    soc.Send(TYPE_DATA_ADD, len(i), i)
  
  ok, digest = soc.Send(TYPE_DATA_DIGEST, 0)
  
  sent_digest = hashlib.md5(chunk).digest()
  if digest == sent_digest:
    return True
  else:
    return False


def UploadBinary(soc, image) :
  size = os.stat(image).st_size
  data = file(image).read()

  parts = DataToChunks(data, 8192)

  # Note that by default you can't use a kernel that overwrites the 
  # config block.  You can still do that by changing this script
  # if you dare - change '119' to '120', and change the g_sectors
  # above.
  if len(parts) > 119:
    raise ValueError("Image size too large!")
    
  for i in g_sectors:
    ok, buf = soc.Send(TYPE_ERASE_SECTOR, i)
    if ok:
      print "Sector %08x erased in %d ticks" % (i,struct.unpack("<L",buf)[0])
    else:
      raise IOError("Flash erase timeout.")

  offset = 0
  for chunk in parts:
    txed = False
    # Try to send the chunk five times.  One of these attempts should succeed.
    for i in xrange(0,5):
      if Transfer(soc, chunk, offset): 
        txed = True
        break
    if not txed:
      raise IOError("Unable to transfer chunk %d (digest error)."%offset)

    ok, err = soc.Send(TYPE_DATA_WRITE, offset)
    if not ok:
      raise IOError("No response to write request at %d" % offset)
    err = struct.unpack("b", err)[0]
    if err:
      raise IOError("Error %d writing data at %d" % (err, offset))
    print "Written chunk at %d" % offset
    offset += 1


def Run(iface, dest, fname):

  soc = BiffSocket(iface, dest, 0xb1ff)

  print "Switch on Bifferboard in the next 10 seconds.  Move it! :)"
  ok = False
  start = time.time()
  while (not ok) and ((time.time() - start) < 10):
    ok, buffer = soc.Send(TYPE_ATTN, 0)
  
  if ok:
    UploadBinary(soc, fname);
    # finally release and allow to boot.
    ok, buffer = soc.Send(TYPE_RELEASE, 0)


if __name__ == "__main__":

  if not sys.argv[3:]:
    print "Usage: bb_eth_upload <interface> <mac address> <firmware>"
    print
    print "<interface>   is the name of your network interface.  Most people"
    print "              will use 'eth0'."
    print "<mac address> should be of the form 6a:b3:f6:19:1d:d6"
    print "              This is on the sticker on your bifferboard."
    print "<firmware>    is the filename of the kernel to flash."
    print
    sys.exit(0)

  dest = GetMac(sys.argv[2])
  Run(sys.argv[1], dest, sys.argv[3])

