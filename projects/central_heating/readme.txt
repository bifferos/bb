Complete central Heating system using the Bifferboard and an Arduino


Ingredients
===========

- Bifferboard (single USB, dual USB, turbo NAS card should all be the same,
1/4/8MB.  This setup will run on anything.
- Arduino
- LCD Shield
- Dallas 1-wire temp sensor
- Latching relay (two control lines required)

You'll have to figure out the wiring I used from the source code.
The LCD shield uses standard pinouts though, I used the SainSmart one.


Software Setup
==============


1) Compile the chmon program with make (expects built OpenWrt toolchain)
2) Compile the Arduino program with Arduino tools and load it to the board.


Linux file layout should look like this:

/chmon
/www/index.html
/www/cgi-bin/message -> /chmon  (soft link)
/www/cgi-bin/therm -> /chmon  (soft link)
/www/cgi-bin/status.sh


Start the system something like this:

httpd -h /www
/chmon /dev/ttyUSB0 &


Of course, you can test the Arduino part from a normal PC, chmon is
statically linked so should run on pretty much anything by intel.  Make
sure busybox httpd has support for cgi.



