Turbo NAS Card

These cards came in a box with the card, cable and PSU adapter.  PSU connector is a floppy-disk style molex.

Spec:
 - 133MHz 486sx CPU equivalent
 - 32MB RAM
 - 1MB NOR Flash

Operating system boot options:
Since the device has only 1MB flash this is limited.
 - 'Tiny' Linux system in the tiny directory of this project fits in 1MB
 - Biffboot recent versions can pull a kernel off a tftp server if configured to do so.
 - CoreBoot is reported to be compatible with these devices so could be used to load an OS from IDE/Sata.
 - Tiny system above can be used to kexec a recent kernel from USB

The back panel has:
 - reset switch (GPIO in)
 - Ethernet RJ45
 - Two LEDs (connected to the network PHY)
 - USB 2.0

On-board the PCB is an LED connected to GPIO.


IO Pins, JTAG:

Removal of R33 results in the JTAG pins (6 pins in a row just below the MAC label) becoming GPIO.  There are four GPIOs, pins 2,3,4,5 (the square PAD indicating pin 1).  These GPIOs are open-collector and will require a weak pull-up.  I don't know if they are 5v tolerant, best to assume not.  Pins 8 and 9 are the serial console.  Any 3.3v CP2102 adapter will communicate with these, pin 6 is ground.  The missing pin 7 is for polarisation and has no function.

IDE/Sata:

These devices are connected via USB => sata/IDE converter.

Edge connector:

This connector is only to provide a mount for the card and carries no signal to the host PC, not even power.
