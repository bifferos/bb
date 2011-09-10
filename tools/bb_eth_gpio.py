#!/usr/bin/env python
"""
    Use the Bifferboard as GPIO-over-ethernet device
"""

import os, time, socket,struct,time,os,sys,string


def GetMac(dotted):
  " '00:00:01:02:03:04' --> '\x00\x00\x01\x02\x03\x04' " 
  dot = dotted.strip()
  bits = dot.split(":")
  if len(bits)!=6:
    return None
  out = ""
  for i in bits:
    if len(i)!=2:
      return None
    if not i[0] in string.hexdigits:
      return None
    if not i[1] in string.hexdigits:
      return None
    out += chr(eval("0x"+i))
  return out


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
TYPE_GPIO=0xe          # GPIO operation (ack)

ERROR_NONE  = 0x80000000
ERROR_VERIFY= 0x80000001
ERROR_FLASH = 0x80000002


# How long in seconds we wait for a response to the various commands 
g_timeout = {
TYPE_ATTN:0.001,
TYPE_RELEASE:0.05, 
TYPE_DATA_WRITE:1.00, 
TYPE_DATA_DIGEST:0.05,
TYPE_ERASE_SECTOR:5.0,
TYPE_GPIO:0.01
}

g_gpio = {
"read" : 0,
"high" : 1,
"low" : 2,
"in" : 3,
"out_high" : 4,
"out_low" : 5
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
        proto = data[12:14]
        if struct.unpack(">H",proto)[0] != 0xb1ff: continue
        src = data[6:12]
        if self.dest == "\xff\xff\xff\xff\xff\xff":
          self.dest = src
        if src != self.dest: continue
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



def Connect(iface, dest):
  print "Waiting to connect to '%s'" % repr(dest)
  
  soc = BiffSocket(iface, GetMac(dest), 0xb1ff)
  while True:
    ok, buffer = soc.Send(TYPE_ATTN, 0)
    if ok:
      print "Connected to '%s'" % dest
      return True
  return False


def Execute(iface, dest, cmnd, pin):
  if not g_gpio.has_key(cmnd): return False
  soc = BiffSocket(iface, GetMac(dest), 0xb1ff)
  
  data = struct.pack("<LLL", g_gpio[cmnd], (1<<int(pin)), 0)
  ok, buffer = soc.Send(TYPE_GPIO, len(data), data)
  if ok:
    operation, mask, result = struct.unpack("<LLL", buffer)
    print "OK", operation, mask, result
  else:
    print "not OK"
  return True


def Help():
  print "Usage: ./bb_eth_gpio.py <iface> <mac addr> connect"
  print "or ./bb_eth_gpio.py <iface> <mac addr> <gpio command> <pin>"
  print "Examples:"
  print "./bb_eth_gpio.py eth0 00:02:03:04:05:06 connect (wait for connection)"
  print "./bb_eth_gpio.py eth0 00:02:03:04:05:06 out_low 16 (LED on)"
  print "./bb_eth_gpio.py eth0 00:02:03:04:05:06 out_high 16 (LED off)"
  print 
  print "GPIO commands:", ", ".join(g_gpio.keys())
  sys.exit(1)


if __name__ == "__main__":
  if not sys.argv[3:]:
    Help()
  cmnd = sys.argv[3]
  if cmnd == "connect":
    Connect(sys.argv[1], sys.argv[2])
  elif Execute(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]):
    pass
  else:
    Help()    
  

