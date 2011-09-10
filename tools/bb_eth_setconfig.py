#!/usr/bin/env python
"""
    GUI for setting config via ethernet
"""

import pygtk
pygtk.require('2.0')
import gtk, gobject
import glob, os, time
import socket,struct,time,os,sys,hashlib, string, threading


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
TYPE_GPIO=0xe        # GPIO operation (ack)

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


def SendChunk(soc, chunk, offset):
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


#def Run(iface, dest, fname):

  
#  if ok:
#    UploadBinary(soc, fname);
#    # finally release and allow to boot.
#    ok, buffer = soc.Send(TYPE_RELEASE, 0)


def GetConfig(bootsource, console, nic, boottype, loadaddress, cmdline, kernelmax):
  "Return 8K config block with correct md5"
  config = struct.pack("<iBBBBL", 1,bootsource,console,nic,boottype,loadaddress)
  
  # int version ==1 for this version.
  # Byte bootsource (0 = flash, 1=MMC)
  # Byte console (1 = serial console enable, 0=disable)
  # Byte nic (0 = net console disabled, doesn't work with Qemu anyhow!)
  # Byte boottype (1 = linux, 0 = flat bin)
  # unsigned long loadaddress (0x400000 default for Linux)
  
  while len(cmdline)<1024:
    cmdline += "\x00"
  block = config + cmdline

  # Comment out this line for early bootloaders  
  block += struct.pack("<H", kernelmax)
  
  while len(block)<(0x2000-0x10):
    block += "\xff"
  
  block += hashlib.md5(block).digest()
  return block
  



class MainGUI:
    def __init__(self):

      self.atten_poll = False
      self.ifaces = [i for i in os.listdir("/sys/class/net") if i!="lo"]
      self.flash_sizes = [1, 4, 8]

      self.soc = None
      self.txt_status = "Disconnected"

      self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
      self.window.set_size_request(500, 400)
      self.window.move(200, 100)
      self.window.set_title("Bifferboard")
      self.window.connect("delete_event", self.destroy)

      # Everything goes in here
      vbox = gtk.VBox(False, 0)
      self.window.add(vbox)
      vbox.show()

      # three values to set
      table = gtk.Table(12, 2, True)

      self.status = gtk.Label(self.txt_status)
      self.AddToTable(table, "Status", 0, self.status)

      self.interface = gtk.combo_box_new_text()
      for i in self.ifaces:
        self.interface.append_text(i)
      self.interface.set_active(0)
      self.AddToTable(table, "Host interface", 1, self.interface)

      self.mac = gtk.Entry()
      self.mac.set_text("00:b3:f6:00:")
      self.AddToTable(table, "Target MAC", 2, self.mac)

      self.flash = gtk.combo_box_new_text()
      for i in self.flash_sizes:
        self.flash.append_text("%d MB" %i)
      self.flash.set_active(2)
      self.AddToTable(table, "Flash Size", 3, self.flash)

      self.config_chunk = gtk.Label("")
      self.AddToTable(table, "Config chunk", 4, self.config_chunk)

      self.cfg_bootsource = gtk.combo_box_new_text()
      for i in ["flash", "MMC", "NET"]:
        self.cfg_bootsource.append_text(i)
      self.cfg_bootsource.set_active(0)
      self.AddToTable(table, "Boot source", 5, self.cfg_bootsource)

      self.cfg_console = gtk.combo_box_new_text()
      for i in ["disabled", "enabled"]:
        self.cfg_console.append_text(i)
      self.cfg_console.set_active(1)
      self.AddToTable(table, "Serial console", 6, self.cfg_console)

      self.cfg_nic = gtk.combo_box_new_text()
      for i in ["disabled", "enabled", "promiscuous"]:
        self.cfg_nic.append_text(i)
      self.cfg_nic.set_active(1)
      self.AddToTable(table, "Network console", 7, self.cfg_nic)

      self.cfg_boottype = gtk.combo_box_new_text()
      for i in ["flat binary", "linux"]:
        self.cfg_boottype.append_text(i)
      self.cfg_boottype.set_active(1)
      self.AddToTable(table, "Boot image type", 8, self.cfg_boottype)

      self.cfg_loadaddress = gtk.Entry()
      self.cfg_loadaddress.set_text("0x400000")
      self.AddToTable(table, "Load address", 9, self.cfg_loadaddress)

      self.cfg_cmndline = gtk.Entry()
      self.cfg_cmndline.set_text("console=uart,io,0x3f8 root=squashfs,jffs2")
      self.AddToTable(table, "Kernel cmndline", 10, self.cfg_cmndline)

      self.cfg_kernelmax = gtk.Entry()
      self.cfg_kernelmax.set_text("")
      self.AddToTable(table, "Kernel max", 11, self.cfg_kernelmax)
        
      table.show()
      vbox.add(table)

      hbox = gtk.HBox(False, 0)
      vbox.add(hbox)
      hbox.show()

      self.connect = gtk.Button("Connect")
      self.connect.connect_object("clicked", self.Connect, self.window)
      hbox.add(self.connect)
      self.connect.set_sensitive(False)
      self.connect.show()


      self.write = gtk.Button("Write")
      self.write.connect_object("clicked", self.Write, self.window)
      hbox.add(self.write)
      self.write.show()

      self.window.show()
        
      self.flash.connect_object("changed", self.ChangeFlashSize, self.window)
      self.ChangeFlashSize(self.window)
        
      #gobject.idle_add(self.OnTimer, None)
      gobject.timeout_add(100, self.OnCheckParamsValid)

    def ChangeFlashSize(self, widget, data=None):
      self.config_chunk.set_text("%d" % self.GetConfigChunk())
      d = {0:"0xf0000", 1:"0x100000", 2:"0x100000"}[self.flash.get_active()]
      self.cfg_kernelmax.set_text(d)
      

    def CheckParams(self):
      iface = self.ifaces[self.interface.get_active()]
      if not iface in self.ifaces:
        return False
      mac = self.mac.get_text()
      dest = GetMac(mac)
      if not dest:
        return False
      return True
      
    def CheckLoadAddress(self):
      txt = self.cfg_loadaddress.get_text().strip()
      if not txt.startswith("0x"):
        return False
      for i in txt[2:]:
        if not i in string.hexdigits:
          return False
      if len(txt) > 10:
        return False
      if len(txt) < 3:
        return False
      return True
      
    def CheckKernelMax(self):
      txt = self.cfg_kernelmax.get_text().strip()
      if not txt.startswith("0x"):
        return False
      for i in txt[2:]:
        if not i in string.hexdigits:
          return False
      if len(txt) > 10:
        return False
      if len(txt) < 7:
        return False
      if not txt.endswith("0000"):
        return False
      return True
      
    def CheckWriteButton(self):
      if self.txt_status != "Connected":
        return False
      if not self.CheckLoadAddress():
        return False
      if not self.CheckKernelMax():
        return False
      return True

    def UpdateWriteButton(self):
      if self.CheckWriteButton():
        self.write.set_sensitive(True)
      else:
        self.write.set_sensitive(False)

    def OnCheckParamsValid(self, data=None):
      if self.CheckParams():
        self.connect.set_sensitive(True)
      else:
        self.txt_status = "Disconnected"
        self.connect.set_sensitive(False)

      self.status.set_text(self.txt_status)
      self.UpdateWriteButton()
      return True

    def GetConfigChunk(self):
      flash = self.flash_sizes[self.flash.get_active()]
      return ((flash * 0x100000) - 0x10000) / 8192 -1

    def PollThread(self):
      dest = GetMac(self.mac.get_text())
      self.soc = BiffSocket(self.ifaces[self.interface.get_active()], dest, 0xb1ff)
      self.txt_status = "Polling"
      while self.atten_poll:
        ok, buffer = self.soc.Send(TYPE_ATTN, 0)
        if ok:
          self.txt_status = "Connected"
          return False  # we're done with this thread
      self.txt_status = "Disconnected"
      self.soc = None

    def Connect(self, widget, data=None):
      # start the polling activity
      thread = threading.Thread(target=self.PollThread)
      self.atten_poll = True
      thread.start()

    def AddToTable(self, table, txt, index, w):
      label = gtk.Label(txt)
      table.attach(label, 0, 1, index, index+1)
      label.show()
      table.attach(w, 1, 2, index, index+1)
      w.show()

    def destroy(self, widget, event, data=None):
      self.atten_poll = False
      gtk.main_quit()

    def Write(self, widget, data=None):
      bootsource = self.cfg_bootsource.get_active()
      console = self.cfg_console.get_active()
      nic = self.cfg_nic.get_active()
      boottype = self.cfg_boottype.get_active()
      loadaddress = eval(self.cfg_loadaddress.get_text())
      cmndline = self.cfg_cmndline.get_text().strip()
      kernelmax = eval(self.cfg_kernelmax.get_text())/0x10000
      
      chunk = GetConfig(bootsource, console, nic, boottype, loadaddress, cmndline, kernelmax)
      SendChunk(self.soc, chunk, self.GetConfigChunk())
      self.txt_status = "Chunk written!"


if __name__ == "__main__":

  d = MainGUI()
  
  gtk.gdk.threads_init()
  gtk.main()

