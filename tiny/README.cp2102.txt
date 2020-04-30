USB serial converters

So-called 'mobile phone data cables' from about 10 years ago are
still being manufactured.  They consist of a USB connector at one 
end, a little box in the middle and and a phone connector at the other
end (which generally gets hacked off on mine).

The box in the middle (sometimes incorporated into the USB connector
itself) contains generally CP2102 or PL2302 USB -> serial converter.
The vast majority of cables work at 5/3.3v levels, making them ideal
for interfacing with microcontrollers. 

Let's ignore the USB-side of the cable.  The peripheral 
side has a number of uses:

TX: For sending data
RX: For reading data
DSR: For reading a logic level
RTS: For sending a logic level.

Since the cables are so cheap they are a great way to add simple
IO functions to a PC.  By using USB and the linux /dev/ttyUSB0 device
one can easily move the cable between different controllers, e.g.
Raspberry Pi/Bifferboard without changing programming.  Also, it adds
a level of protection over simply using direct GPIO on the m/board which 
may get incorrectly connected causing damage.

The obvious application for the DSR is with a button, e.g. a doorbell.
The RTS can be used for a buzzer.

My cable is wired as follows:
Yellow - 5v (but only with the switch on the side of the box in one
         position.
Green - RTS (can be controlled)
Red - GND
White - TX
Blue - RX
