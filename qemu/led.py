#!/usr/bin/env python

# LED peripheral emulation code.

import asyncore, panel

g_StateMap = {
   "0" : "on",
   "1" : "off",
   "P" : "off",
   "X" : "off",
   "E" : "in contention!"
}

LED_PIN = 0

class LED(panel.Peripheral):
  def __init__(self):
    panel.Peripheral.__init__(self, [LED_PIN])
    self.InitialValue = None
    
  def pins_updated(self, states):
    # The framework only delivers changes to the pins we requested.
    state = states[0]
    print "LED is now: %s (%s)" % (g_StateMap[state], state)


if __name__ == "__main__":
  LED()
  asyncore.loop()

