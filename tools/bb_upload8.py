#!/usr/bin/env python
"""
   Automatically flash kernel via Biffboot bootloader


   Prerequisites:
  
   - 3.3v Serial data cable to connect to Bifferboard.  I am using a Siemens S55 
   USB mobile phone data cable.  This seems to work nicely and be the least
   hassle.  You can also buy an RS232 level shifter.  Note that if you connect 
   RS232 signal levels directly to the Bifferboard you will probably destroy it!!!

   - Linux (of course...)
   - Python 2.5, it may well work with 2.4.
   - Pexpect.  Please get it from http://sourceforge.net/projects/pexpect/,
   download and unpack the tarball then run 'python setup.py install' (as root).


   Usage Instructions

   Ensure the kernel image is called 'firmware.bin', and that device 
   /dev/ttyUSB0 exists, and then run this script.  Switch on the Bifferboard
   and transmission should start.  You will get a display of progress, this
   is a count of the number of 8192-byte chunks of kernel which have been
   programmed.  You can do the math to work out when it will complete :).

   The display should stop updating every second when the kernel has been 
   transfered.  At this point you will be dropped into a terminal with access 
   to the bootloader - hit <ENTER> to display the menu, and press '4' to boot.


   Data error detection

   In practice, no transmission errors have been observed when sending
   data at 115200 baud over a cable length of 2 feet, so there is no 
   error correction or recovery, only error *detection*.  The purpose
   of error detection is to avoid writing an invalid firmware without 
   you realising.  It won't save you time:  In the event of an error 
   you will need to start over again with the transfer.  So far I've 
   never needed to do this, hence have not written any recovery 
   action into this script.  1MB @115200 baud doesn't take too long to
   send anyhow.


   Internal operation

   The bootloader is able to accept data in packets.  Packets consist of
   four dashes (----) followed by an md5 digest followed by a payload.
   Packet transmission is started by choosing the appropriate bootloader
   menu option and then sending the data.  The payload size has been chosen
   as 8192 bytes in order than each packet takes approximately one
   second to be transmitted.  This allows a display of progress.  After 
   successful reception of a packet a further bootloader option can be 
   selected to write it to one of the flash sectors.  The bootloader 
   itself keeps no count of current sector - that is specified every time
   with this script, which keeps the bootloader simple.

   Note there are only 8192-byte packets - shorter packets are padded with
   0xFF.
   

   bifferos@yahoo.co.uk  2008
"""
import fdpexpect, os, sys, tempfile, struct, termios, time
import string, StringIO, os, hashlib
    

def SetupSerial(device) :
  print "Setting device '%s' to 115200, 8N1" % device
  fd = os.open(device, os.O_RDWR)
  params = termios.tcgetattr(fd)
  params[0] = termios.IGNBRK        # iflag, 1
  params[1] = 0                     # oflag, 0
  # cflag:  0x18b2
  params[2] = (termios.CS8 |     # 0x30    8-bit data
              termios.CLOCAL |   # 0x800   ignore modem ctrl lines.
              termios.CBAUDEX |  # 0x1000
              termios.CREAD |    # 0x80
              params[2] & 2)     # leave bit 2 as-is.
  params[3] = 0                  # lflag
  params[4] = termios.B115200   # ispeed
  params[5] = termios.B115200   # ospeed
  # leave cc flags as-is.
  termios.tcsetattr(fd, termios.TCSANOW, params)



class Power:
  """rb_power is an external utility to control power to the board.
  """
  def __init__(self):
    self.name = "rb_power"
    self.has_power = False
    for i in os.environ["PATH"].split(":"):
      if not os.path.isdir(i):
        continue
      for file in os.listdir(i):
        if self.name==os.path.split(file)[1]:
          self.has_power = True

  def On(self):
    if self.has_power:
      os.system(self.name+" on")
    else:
      print "Please power on the board (power off first if already on)"
      
  def Off(self):
    if self.has_power:
      os.system(self.name+" off")
      time.sleep(0.5)


g_power = Power()


def Initialize(device):
  "Set up the board to a known state and connect pexpect to it.  Return the pexpect object."
  g_power.Off()

  SetupSerial(device)

  print "Connecting to serial device...",
  fd = os.open(device, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY)

  print fd
  m = fdpexpect.fdspawn(fd)


  print "Waiting for embedded device to be switched on..."
  g_power.On()
  return m


def Terminal(device):
  "Reconnect to serial device and give the user a nice terminal to watch the boot"
  fd = os.open(device, os.O_RDWR|os.O_NONBLOCK|os.O_NOCTTY)
  m = fdpexpect.fdspawn(fd)
  m.interact()


# Return the individual chunks.
def FirmwareToPackets(data):
  out = []
  index = 0
  csize = 8192
  count = 0
  maxb = len(data)/8192
  if len(data)%8192:
    maxb += 1
  max_txt = struct.pack("<I", len(data))
  padding = 0
  for index in xrange(0, len(data), csize):
    chunk = data[index:index+csize]
    while len(chunk)<csize:
      padding += 1
      chunk += "\xFF"
    payload = chunk
    digest = hashlib.md5(payload).digest()
    entry = "----" + digest + payload
    out.append(entry)
  return out, padding


def PacketsToFirmware(parts, padding):
  chunks = [ i[20:] for i in parts ]
  data = "".join(chunks)
  data = data[:-padding]
  return data



def Tester():
  print "Start init"
  count = 0
  for i in chunk:
    m.write(i)
    out = m.read(1)
    if out!=i:
      print "Found bad character"
    if (count%0x10)==0:
      print count
    count += 1

  print "done chunk"
  sys.exit()


def Flash(m, chunk, offset):
  m.send("2")
  m.expect("Waiting for data...")
  #print "Target waiting for data"

  time.sleep(0.01)
  for i in xrange(0,len(chunk), 256):
    m.write(chunk[i:i+256])
  #print "Chunk sent"

  m.expect("BIFFBOOT> ")
  #print "Chunk received"
  m.send("3")
  m.expect("BIFFBOOT> ")
  m.send("%03d" % offset)

  m.expect("Programmed OK")
  m.expect("BIFFBOOT> ")
  print "Programmed chunk at offset %03d" % offset



def UploadBinary(device, image) :

  size = os.stat(image).st_size
  data = file(image).read()

  parts,pad = FirmwareToPackets(data)

  m = Initialize(device)

  print "Waiting for device to power up"
  m.expect("ESC")
  print "Found 'ESC'"
  m.send("\x1b")
  m.expect("BIFFBOOT> ")
  print "Menu printed"

  m.send("1")
  print "Erasing flash"
  m.expect("BIFFBOOT> ")

  offset = 0
  for chunk in parts:
    Flash(m, chunk, offset)
    offset += 1

  print "Done programming, Press '4' to boot kernel"
  del m
  Terminal(device)


g_default_kernel = "openwrt/bin/openwrt-bifferboard.bzImage"

def Usage():
  print "Usage: bb_upload.py <serial device> [<kernel>]"
  print
  print "Used to upload a kernel to the Bifferboard over the serial port."
  print 
  print "<serial device> is the device under /dev that the Bifferboard is"
  print "        connected to, e.g. /dev/ttyUSB0"
  print "<kernel> is the path to the kernel to upload.  If not given this"
  print "        defaults to '%s'" % g_default_kernel
  print
  sys.exit(0)


if __name__ == "__main__":
  if not sys.argv[1:]:
    Usage()
  if sys.argv[3:]:
    Usage()
  if sys.argv[2:]:
    kern = sys.argv[2]
  else:
    kern = g_default_kernel
  if not os.path.isfile(kern):
    print "Unable to open kernel '%s'" % kern
    Usage()
  UploadBinary(sys.argv[1], kern)



