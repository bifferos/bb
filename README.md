Introduction

See https://sites.google.com/site/bifferboard/ for more details.

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
 

