#!/usr/bin/env python

# Emulation of a button, shown in a tkinter interface. 

import panel
from Tkinter import *

bhelp = "Button State.  Check to push putton, uncheck to release button"


PANEL_PIN = 1

class App:
  def __init__(self, master):
    frame = Frame(master)
    frame.pack()
    self.var = StringVar()
    self.button = Checkbutton(frame, onvalue="0", offvalue="P", 
      variable=self.var, command=self.say_cb, text=bhelp)
    self.button.deselect()
    self.button.pack(side=LEFT)
    self.panel = panel.Actor()
    self.panel.SetPin(PANEL_PIN, "P")    # resistor pull-up.

  def say_cb(self):
    self.panel.SetPin(PANEL_PIN, self.var.get())

if __name__ == "__main__":
  root = Tk()
  app = App(root)
  root.mainloop()

