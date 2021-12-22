Introduction

See https://sites.google.com/site/bifferboard/ for more details.

The Bifferboard was a tiny 486-compatible SBC I sold back around 2011.   This is the software for it.
I still have a supply of Bifferboards which I use for various home-automation tasks.  Although I also have a few
Raspberry Pis I found them to not be as reliable as the WANSER-R based hardware in the Bifferboard.  I think
it may just be that the Biffer has on-board NOR flash which I tend not to write to, most of my Biffer systems
use custom-compiled initrd and just run in RAM doing their thing.  I never did like SD-cards but boards like the
Beaglebone Black with on-board flash are too pricey.

If you still want to get the hardware please get in touch as I have a fair few of these spare.
I doubt I'll find a use for all of them.


Instructions for use (Compiling)

This was originally developed on Slackware circa 13.1, however that Slack version does not boot on VirtualBox 6.0.12.
Use instead Slackware 13.37.  Slackware 14.0 is too late and some openwrt compile errors start to emerge at that point 
(probably fixable but I don't have the time...).  

Slackware 13.37 is incapable of connecting to github and downloading this software you'll have to scp it across from
a more recent machine.

BEFORE YOU START:  You will need to download kernel linux-2.6.37.6.tar.bz2 and put it in the openwrt/dl directory 
because the OpenWrt have removed it from their site.  Otherwise the compile seems to work as of July 2021.

To build Bifferboard firmware and emulator type 'make'.

To run the image under the emulator type 'make run'.
CTRL-A, X quits the emulator.
There is some kind of bug in the NOR flash detection which I haven't figured out.  It may be as simple as changing 
Qemu to return the correct flash code but it does at least run.  That version of Qemu should also be capable of running
the bzImage correctly, may be quicker than fixing the firmware image loading if that's all that's needed.

Edit the Makefile to suit how you want to organise your virtual
network.  See Qemu documentation at http://qemu.weilnetz.de/qemu-doc.html

Vagrant

Vagrant is a tool from Hashicorp allowing you to manage virtual machines.  Hashicorp also allows one to host public machines
for free on Vagrant cloud.  To make it easier to work with the Bifferboard software I've created a virtual machine with 
Slackware 13.37.  To use it:

 - Install VirtualBox
 - Install vagrant
 - vagrant up
 - vagrant ssh
 - cd /vagrant
 - cp /usr/src/linux-2.6.36.7.tar.bz2 openwrt/download/.
 - make
 
[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=5UT56VZB37SNL)

