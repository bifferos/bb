#!/usr/bin/env python

""" 
   Send characters to LCD connected to the Panel using HD44780 protocol
   Note that we don't use any delays here and just assume that the 
   connected LCD is an emulation, and therefore not critical to timing.
   It is quite possible that a real display could be connected to the 
   other side of the Panel (or more specifically a proxy client for a
   real display) in which case some time delays will have to be
   introduced.

"""

import panel
from Tkinter import *

# Panel pins to use.
HD_E  = 0
HD_RS = 1 
HD_DB4= 2
HD_DB5= 3
HD_DB6= 4
HD_DB7= 5

class LCD:
  def __init__(self):
    self.panel = panel.Actor()
    
    self.WriteNibble(0x03)
    self.WriteNibble(0x03)
    self.WriteNibble(0x03)
    self.WriteNibble(0x02)

    self.WriteCommand(0x28)
    self.WriteCommand(0x0c)
    self.WriteCommand(0x01)
    self.WriteCommand(0x06)


  def HD_RS_LOW(self):
    self.set_value(HD_RS, 0)
    
  def HD_RS_HIGH(self):
    self.set_value(HD_RS, 1)
    
  def HD_E_LOW(self):
    self.set_value(HD_E, 0)
    
  def HD_E_HIGH(self):
    self.set_value(HD_E, 1)

  def set_value(self, pin, val):
    if val==0:
      self.panel.SetPin(pin, "0")
    else:
      self.panel.SetPin(pin, "1")
    
  def WriteNibble(self, val):
    self.set_value(HD_DB4, val & 1)
    self.set_value(HD_DB5, (val & 2)>>1)
    self.set_value(HD_DB6, (val & 4)>>2)
    self.set_value(HD_DB7, (val & 8)>>3)
    self.HD_E_LOW()
    self.HD_E_HIGH()

  def WriteData(self, c):
    self.HD_RS_HIGH()
    self.WriteNibble((c>>4) & 0x0f)
    self.WriteNibble(c & 0x0f)

  def WriteCommand(self, c):
    self.HD_RS_LOW()
    self.WriteNibble((c>>4) & 0x0f)
    self.WriteNibble(c & 0x0f)

if __name__ == "__main__":
  lcd = LCD()
  for i in "Hello world!":
    lcd.WriteData(ord(i))
