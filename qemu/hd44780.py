#!/usr/bin/env python
"""
   Emulate hd44780 display.  This is a very crude emulation of the protocol, many 
   improvements could be made.
"""

import panel, asyncore, lcdfont
import pygtk
pygtk.require('2.0')
import gtk,gobject
import sys


# The LED pins used
HD_E  = 0
HD_RS = 1 
HD_DB4= 2
HD_DB5= 3
HD_DB6= 4
HD_DB7= 5

pin_map = [HD_E, HD_RS, HD_DB4, HD_DB5, HD_DB6, HD_DB7]


class PanelEvents(panel.Peripheral):
  def __init__(self, lcd_out, init_txt):
    panel.Peripheral.__init__(self, pin_map)
    self.prev_E = "1"
    self.prev_cmnd = []
    self.prev_data = []
    self.lcd_init = False
    self.lcd_out = lcd_out  # write the new text.
    self.lcd_text = init_txt
    self.lcd_cursor = 0


  def lcd_update(self, newchar):
    self.lcd_text = self.lcd_text[:self.lcd_cursor]+chr(newchar)+self.lcd_text[self.lcd_cursor+1:]
    self.lcd_text = self.lcd_text[:32]
    self.lcd_cursor += 1
    if self.lcd_cursor > 31:
      self.lcd_cursor = 0
    self.lcd_out(self.lcd_text)

    
  def pins_updated(self, states):
    "Nothing even approaching correct emulation, but who gives a stuff"
    E,RS,DB4,DB5,DB6,DB7 = states
    val = 0
    for i in [DB7, DB6, DB5, DB4]:
      val <<= 1
      if i!="0":
        val += 1
    if (E!="0") and (self.prev_E == "0"):
      if RS == "0":
        self.prev_cmnd.append( val )
        if len(self.prev_cmnd)>1:
          high = self.prev_cmnd[0]
          low = self.prev_cmnd[1]
          total = (high<<4)|low
          if total == 0x06:
            self.lcd_init = True    # reset condition.
            self.lcd_cursor = 0
            self.lcd_text = "\x00" * 32
          self.prev_cmnd = []
        self.prev_data = []
      else:
        self.prev_cmnd = []
        # Data value
        if self.lcd_init:
          self.prev_data.append( val )
          if len(self.prev_data)>1:
            high = self.prev_data[0]
            low = self.prev_data[1]
            total = (high<<4)|low
            if self.lcd_init:
              self.lcd_update(total)
            self.prev_data = []

    self.prev_E = E


class LCD:

    def __init__(self):
        # Connect to the panel to get the panel events.
    
        window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        window.connect("delete_event", self.delete_event)
        window.connect("destroy", self.destroy)
        window.set_border_width(10)
        window.show()   # show window
        
        vbox = gtk.VBox(False,0)
        window.add(vbox)
        vbox.show()
        self.vbox = vbox
        
        area = gtk.DrawingArea()
        area.set_size_request(400, 116)
        vbox.pack_start(area, True, True, 0)
        area.show()
        
        area.connect("expose_event", self.expose_event)
        area.connect("configure_event", self.configure_event)

        # colours
        self.pixelon = area.window.new_gc()
        self.pixelon.set_rgb_fg_color(gtk.gdk.color_parse("#000000"))
        self.pixeloff = area.window.new_gc()
        self.pixeloff.set_rgb_fg_color(gtk.gdk.color_parse("#949494"))
        self.background = area.window.new_gc()
        self.background.set_rgb_fg_color(gtk.gdk.color_parse("#c6c6c6"))
        self.white = area.window.new_gc()
        self.white.set_rgb_fg_color(gtk.gdk.color_parse("#ffffff"))
        self.shade = area.window.new_gc()
        self.shade.set_rgb_fg_color(gtk.gdk.color_parse("#dedede"))
        
        self.window = window
        self.area = area
        self.vbox = vbox
        
        self.pixmap = None
        self.font = lcdfont.Font5x7()
        self.pixel = 4
        self.bezel = 18
        
        self.lcd_text = "\x00" * 32 
        panel = PanelEvents(self.repaint, self.lcd_text)
        gobject.io_add_watch(panel.socket, gobject.IO_IN|gobject.IO_HUP,self.poll_now)
        for i in range(10):
          asyncore.poll()  # make the connection
        
    def poll_now(self, fd, arg2):
        asyncore.poll()
        return True

    def delete_event(self, widget, event, data=None):
        return False  # allow exit
        
    def destroy(self, widget, data=None):
        gtk.main_quit()

    def expose_event(self, widget, event):
        x, y, width, height = event.area
        widget.window.draw_drawable(widget.get_style().fg_gc[gtk.STATE_NORMAL],
                                self.pixmap, x, y, x, y, width, height)
        return False
                                            
    def configure_event(self, widget, event):
        x, y, width, height = widget.get_allocation()
        self.pixmap = gtk.gdk.Pixmap(widget.window, width, height)
        self.pixmap.draw_rectangle(self.background,
                                         True, 0, 0, width, height)
        self.repaint(self.lcd_text)
        return True
        
    def repaint(self, lcd_text):
        widget = self.area
        # convert chars to display pixels.
        self.lcd_text = lcd_text
        chars = self.lcd_text
        #for char_ofs in xrange(0,len(chars)):
        pixel = self.pixel
        sep = 2
        cwidth = pixel*5+sep
        xpos = self.bezel + 8
        ypos = self.bezel + 8
        line_count = 0
        for char in chars:
          ras = 0
          for line in self.font.Get(ord(char)):
            for bit in xrange(0,5):
              x = xpos+(bit*pixel)
              y = ypos+ras*pixel
              if line[bit] == "@":
                self.pixmap.draw_rectangle(self.pixelon, True, x, y, pixel, pixel)
              else:
                self.pixmap.draw_rectangle(self.pixeloff, True, x, y, pixel, pixel)
            ras += 1
          xpos += cwidth
          line_count += 1
          if line_count > 15:
            line_count = 0
            ypos += pixel*8+sep
            xpos = self.bezel + 8
          
        self.repaint_frame(widget)
        
        x, y, width, height = widget.get_allocation()
        self.area.queue_draw_area(x, y, width, height)

    def repaint_frame(self, widget):
        x, y, w, h = widget.get_allocation()
        # pixel off along the botton
        p = 2
        bezel = self.bezel
        #yin = 6*p
        self.pixmap.draw_rectangle(self.pixeloff, True, 0, h-p, w, p)
        self.pixmap.draw_rectangle(self.pixeloff, True, w-p, 0, p, h)
        self.pixmap.draw_rectangle(self.white, True, 0, 0, w, p)
        self.pixmap.draw_rectangle(self.white, True, 0, 0, p, h)

        # horizontal lines bottom of bezel
        self.pixmap.draw_rectangle(self.white, True, bezel, h-bezel, w-2*bezel+p, p)
        self.pixmap.draw_rectangle(self.shade, True, bezel+p, h-bezel-p, w-2*bezel, p)
        
        # Vertical lines, right
        self.pixmap.draw_rectangle(self.white, True, w-bezel, bezel, p, h-2*bezel)
        self.pixmap.draw_rectangle(self.shade, True, w-bezel-p, bezel+p, p, h-2*bezel-2*p)
        
        # horizontal lines top of bezel
        self.pixmap.draw_rectangle(self.pixeloff, True, bezel, bezel, w-2*bezel, p)
        self.pixmap.draw_rectangle(self.pixelon, True, bezel+p, bezel+p, w-2*bezel-2*p, p)

        # Vertical lines, left
        self.pixmap.draw_rectangle(self.pixeloff, True, bezel, bezel, p, h-2*bezel)
        self.pixmap.draw_rectangle(self.pixelon, True, bezel+p, bezel+p, p, h-2*bezel-2*p)

    def main(self):
        gtk.main()

if __name__ == "__main__":
    hello = LCD()
    hello.main()
