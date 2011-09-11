#!/usr/bin/env python

"""
  GPIO Panel server

  Processes can connect to this server to influence or read the state of 
  GPIO pins.  The socket connection mirrors the physical connection of 
  devices.  When writing values, a table is simply updated with the
  last values written.  When reading values back, an arbitration process
  is used to determine the value that will be returned.
  
  The number of pins handled by the server is set below (PIN_COUNT).
  Clients attempting to access outside this range will trigger exceptions.
  
  The server recognises a number of different pins states as follows:
  
  '1' Logic '1', pin actively pulled high
  '0' Logic '0', pin actively pulled low
  'P' Logic '1', pin passively pulled high (via pull-up resistor)
  'X' Tri-stated, pin either not connected, or high impedance
  'E' Contention resulting in an indeterminate state
  
  When clients initially connect to the server, they implicitly set all 
  pins to 'X' (not connected/input).  They will normally then issue commands
  to change any connected pins to the appropriate values.

  To avoid excessive polling, clients may register for state changes.
  
  Protocol
  
  All commands use a single newline as terminator.
  
  At any time, clients may register for state changes on a particular pin
  with the 'NOTIFY' command, followed by a space, followed by the name of
  the pin to notify on.  It is not possible to de-register from 
  notifications.

  Pin values are changed with the W command.  'W' is followed by a space 
  then a pin id, then another space, then either P, 1, 0 or X.
  
  Pin values are read with the 'R' command.  The response is a letter 'R' 
  followed by a space, and then a series of letters indicating the pin values
  after any arbitration.  Conflicting pins are indicated with an 'E', and it's
  expected that the clients will signal to the user that a conflict has 
  occured.

  bifferos@yahoo.co.uk  
"""


import asyncore, asynchat
import os, socket, string, sys
import re

# 8 GPIOs on the Bifferboard.
PIN_COUNT = 8
GPIO_HOST = "localhost"
GPIO_PORT = 0xb1ff


class Channel(asynchat.async_chat):
  def __init__(self, sock, server):
    asynchat.async_chat.__init__(self,sock)
    self.set_terminator("\r\n")
    self.header = None
    self.data = ""
    self.shutdown = 0
    self.notify = []
    self.server = server
    self.server.clients[sock.fileno()] = self
    # re for setting pins
    self.wmatch = re.compile(r"W (\d{1,3}) ([10PX])")
    # re for notifications
    self.nmatch = re.compile(r"NOTIFY (\d{1,3})")
    self.pins = self.server.GetDefaultPins()
    
  def collect_incoming_data(self, data):
    self.data = self.data + data
      
  def found_terminator(self):
    if self.data.startswith("W "):
      m = self.wmatch.match(self.data)
      if m:
        pin, val = m.groups()
        pin = int(pin)
        if pin < PIN_COUNT:
          self.server.clients[self.socket.fileno()].SetPin(int(pin), val)
          self.server.Resolve()
    elif self.data == "R":
      self.NotifyValues(self.server.Resolve())
    elif self.data.startswith("NOTIFY "):
      m = self.nmatch.match(self.data)
      if m:
        pin = int(m.groups()[0])
        if pin < PIN_COUNT:
          self.notify.append(pin)
    else:
      print "Received invalid command", repr(self.data)
    self.data = ""

  def handle_close(self):
    # Remove from map.
    del self.server.clients[self.socket.fileno()]
    # Someone left the party?  The levels might change as a result
    self.server.Resolve()
    self.close()

  def NotifyValues(self, txt):
    self.push("R "+txt+"\r\n")  

  def SetPin(self, pin, val):
    "Used both for state changes, and (dis)connection."
    self.pins[pin] = val
    
  def GetPin(self, pin):
    "Return an individual pin state, as input for the arbitration process"
    return self.pins[pin]
    
    
class TerminalBlock(asyncore.dispatcher):
  def __init__(self):
    asyncore.dispatcher.__init__(self)
    self.port = GPIO_PORT
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) 
    self.bind(("", self.port))
    self.listen(5)
    # Map of connection:con_object
    self.clients = {}

    # Arbitration logic table between two pins.
    self.PinArbitration = {
      #   :  10PX
      "1" : "1E11",
      "0" : "E000",
      "P" : "10PP",
      "X" : "10PX"
    }
    self.LastResolve = self.GetDefaultPins()

  def handle_accept(self):
    conn, addr = self.accept()
    Channel(conn, self)

  def ResolveTwo(self,pinA, pinB):
    "Given a couple of pin client states, work out the outcome between them"
    return self.PinArbitration[pinA]["10PX".find(pinB)]
    
  def ResolvePin(self,pin):
    "Resolve a single pin across all clients"
    plist = [self.clients[i].GetPin(pin) for i in self.clients.keys()]
    if len(plist)==0: return 'X'  # no clients?
    while len(plist)>1:
      p = self.ResolveTwo(plist[0], plist[1])
      del plist[0]
      plist[0] = p
    return plist[0]
    
  def NotifyAll(self, vals, diffs):
    for i in self.clients.keys():
      obj = self.clients[i]
      notify = False
      for i in obj.notify:
        if i in diffs:
          obj.NotifyValues(vals)
          break
    
  def Resolve(self):
    out = ""
    for i in xrange(0,PIN_COUNT):
      out += self.ResolvePin(i)
    if out != self.LastResolve:
      # Get list of differences
      diffs = []
      for i in xrange(0,len(out)):
        if out[i] != self.LastResolve[i]:
          diffs.append(i)
      self.LastResolve = out
      self.NotifyAll(out, diffs)
    return out

  def GetDefaultPins(self):
    pins = []    
    for i in xrange(0,PIN_COUNT):
      pins.append('X')
    return pins



"""
   Generic peripheral helper class.  Deals with setting up notifications
   and provides a framework for the peipheral implementation
"""

class Peripheral(asynchat.async_chat):

  def __init__(self, pinlist):
    asynchat.async_chat.__init__(self)
    self.set_terminator("\r\n")
    self.data = ""
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.connect((GPIO_HOST, GPIO_PORT))
    for i in pinlist:
      self.push("NOTIFY %d\r\n" % i)   # Subscribe to pin notifications
    self.push("R\r\n")   # Push a query so we get the initial pin state
    self.pinlist = pinlist    # pin(s) we're connected to.

  def handle_connect(self):
    pass

  def handle_expt(self):
    self.close()

  def collect_incoming_data(self, data):
    self.data = self.data + data

  def found_terminator(self):
    # got a response line
    data = self.data
    self.data = ""

    if data.startswith("R "):
      states = []
      pin = data[2:]
      for i in self.pinlist:
        states.append(pin[i])
      self.pins_updated(states)
    else:
      print "Ignoring:", data
      
  def pins_updated(self, states):
    print "New pin values", states
    print "Override this method to do something useful"
    
  def pin_set(self, pin, val):
    self.push("W %d %s\r\n" % (pin,val)) 


class Actor:
  def __init__(self):
    self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.s.connect((GPIO_HOST,GPIO_PORT))
    
  def SetPin(self, pin, val):
    if pin < PIN_COUNT:
      self.s.send("W %d %s\r\n" % (pin, val))



if __name__ == "__main__":

  s = TerminalBlock()
  print "Serving access to GPIO Panel"
  asyncore.loop()


