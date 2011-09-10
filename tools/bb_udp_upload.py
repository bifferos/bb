#!/usr/bin/env python

"""   
   Bifferboard network flashing script, portable version.

   To be used with Biffboot v3.4 onwards.
   
   This is a UDP version of the bb_eth_upload8.py script previously
   used with the Bifferboard. The protocol is the same, but with
   an IP and UDP header added.
   
   Hold in the button on the board, switch it on, then it should
   start a UDP server.
"""

import socket,struct,time,os,sys,hashlib, binascii
from StringIO import StringIO


# Ether protocol command-codes.
TYPE_ATTN=0        # console break-in packet.  (ack)
TYPE_RELEASE=2     # allow execution to continue. (ack)
TYPE_DATA_RESET=4  # reset data receive buffer (no ack)
TYPE_DATA_ADD=6    # send data (no ack).
TYPE_DATA_DIGEST=8   # md5 the data in buffer (md5 returned)
TYPE_DATA_WRITE=0xa  # write block to flash (ack)
TYPE_ERASE_SECTOR=0xc  # erase flash (but not config block) (ack)


# How long in seconds we wait for a response to the various commands 
g_timeout = {
TYPE_ATTN:0.001,
TYPE_RELEASE:0.05, 
TYPE_DATA_WRITE:1.00, 
TYPE_DATA_DIGEST:0.05,
TYPE_ERASE_SECTOR:5.0
}


class BiffSocket:

  def __init__(self, dest):
    soc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    soc.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    soc.bind( ("", 0xb1fe) )
    soc.settimeout( 2.0 )
    self.soc = soc
    self.dest = dest


  def Receive(self, cmd):
    data = "----"
    while data:
      try:
        data, addr = self.soc.recvfrom(1000)
        if len(data)<6: continue
        val, arg = struct.unpack("<HL", data[:6])
        buffer = data[6:]
        if val != (cmd | 1): continue
        # the response we're looking for
        
        # If we had to broadcast the ATTN, then stop broadcasting the
        # rest of the conversation
        if self.dest == "255.255.255.255":
          print "Found Bifferboard at address '%s'" % addr[0]
          self.dest = addr[0]
        return (arg, buffer)
      except socket.error:
        data = ""
    return (None,None)


  def Send(self,cmd,arg, data=""):
    cmd_p = struct.pack("<H",cmd)
    arg_p = struct.pack("<L",arg)
    self.soc.sendto(cmd_p + arg_p + data, (self.dest, 0xb1ff))
    if cmd in g_timeout.keys():
      count = g_timeout[cmd]
      while count>0:
        time.sleep(0.001)   # wait for response
        count -= 0.001
        arg, buffer = self.Receive(cmd)
        if arg!=None:
          return (True, buffer[:arg])

    return (False, "")


def Transfer(soc, chunk, offset):
  soc.Send(TYPE_DATA_RESET, 0)
  fp = StringIO(chunk)
  while 1:
    data = fp.read(512)
    if data == "": break
    soc.Send(TYPE_DATA_ADD, 512, data)
  
  ok, digest = soc.Send(TYPE_DATA_DIGEST, 0)
  
  sent_digest = hashlib.md5(chunk).digest()
  if digest == sent_digest:
    return True
  else:
    #print "Digest mismatch"
    #print "Sent:", binascii.hexlify(sent_digest)
    #print "Received:", binascii.hexlify(digest)
    return False


def UploadBinary(soc, image) :
  size = os.stat(image).st_size
  
  fp = file(image, "rb")

  offset = 0
  first = True
  while 1:
    chunk = fp.read(8192)
    if len(chunk)==0 : break
    while len(chunk)<8192:
      chunk += "\xff"

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
    if first:
      sys.stdout.write("Written chunk at    ")
      first = False
    sys.stdout.write("\b\b\b")
    sys.stdout.write("%03d" % offset)
    sys.stdout.flush()
    offset += 1
  print


def Run(fname, dest):

  soc = BiffSocket(dest)

  ok, buffer = soc.Send(TYPE_ATTN, 0)
  
  if ok:
    UploadBinary(soc, fname);
    # finally release and allow to boot.
    ok, buffer = soc.Send(TYPE_RELEASE, 0)
    if ok:
      print "Released OK"
    else:
      print "Error releasing"
  else:
    print "No board responded to supplied address (%s)" % dest


if __name__ == "__main__":

  if not sys.argv[1:]:
    print "Usage: bb_udp_upload <firmware> [ip address]"
    print
    print "<firmware>    is the filename of the kernel to flash."
    print "[IP address]  Optional IP address of the Bifferboard"
    print
    sys.exit(0)

  if sys.argv[2:]:
    fname = sys.argv[2]
  else:
    fname = "255.255.255.255"
    
  Run(sys.argv[1], fname)

