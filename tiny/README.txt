Modifications to busybox and linux that allow really tiny systems on the
Bifferboard, a device with 1MB flash and 32MB DRAM.

2021 Update:
Although I have several bifferboards still (yes, even after all this time!)
I only ever bother with 1MB firmware, because it's too bothersome to do 
different things for 8MB vs 1MB firmware.  I'd have to deal with JFFS vs 
non-JFFS for instance, and I don't want to do that.  So this is the only 
firmware I create, and it's generally used for some specific task in home
automation.  The Bifferboards continue to be ultra reliable, far more so
than RPi, I don't really know why perhaps the barrel connector helps with
a stable power supply.  In conjunction with an aduino Nano or a USB-serial
converter you can create quite a few projects.  The Bifferboard basically 
acts as a network stack, and it seems to work significantly better than 
Arduino + Network shield.  You cannot however use a more recent kernel, so 
you're stuck with any issues in Kernel 2.6.  It may be possible to hack a
newer kernel into such a small size with config changes, but this is a 
project in itself and I don't have time to do this.  You could also kexec 
another kernel in theory.

Use of Kernel 2.6 requires some extra steps to get a compilation, because
recent compilers cannot compile a kernel this old.

We will use buildroot for the toolchain, however we need to use an old 
version of buildroot to get the correct compiler version to compile the 
Linux kernel.  Using a sufficiently old version of buildroot means we can
no longer compile it on the latest Slackware (or probably any recent Linux).

The following strategy has been used for Linux Mint 20.1 Cinnamon.  It should
also work on Slackware 14.2.

- Obtain Slackware 13.37 (64-bit version), perform full OS install in VM.
- Obtain buildroot 2011.11
   https://buildroot.org/downloads/buildroot-2011.11.tar.bz2
- Run 'make menuconfig'
- Select Target Architecture i386
- Select Target Arhcitecture variant i486
- All other options can remain as defaults.
- The make operation will complain about '.' being in the path, simply edit it
  out of /etc/profile
- Once the build has completed, transfer the output directory over to your 
  64-bit desktop machine and put the compiler in 
  ./output/host/usr/bin/i486-unknown-linux-uclibc-*
  This should run without issue as most compiler files just depend on glibc 
  so will be happy.
- One exception is cc1 which depends on mpfr, so make a link from 
  mpfr.so.4 -> mpfr.so.6 in /usr/lib64.  mpfr v6 seems to be backward
  compatible.
- Put the built toolchain in ../../buildroot-2011/11/output/host
  You should be able to now use this to compile busybox 1.22.1 provided here.
- For the kernel you may get an error from Perl at line 373 in the kernel file kernel/timeconst.pl
  If you do, simply do what it says, take out the defined, e.g. 
     if (!defined(@val)) {  
  becomes 
     if (!@val) {
  This was required for Linux Mint.

For booting the bifferboard with this firmware (bzImage), you should use a command-line such as:
console=uart,io,0x3f8 biffip=172.17.0.15

DHCP kernel support takes up a fair amount of space, so it's better to
configure the IP address via biffip, which is picked up by biffparam.

There isn't space for openssl in 1MB of flash, so busybox runs a telnet
daemon.  There is no security at all, not even a password.  Likewise with
the ftp server.  This allows uploading of files and running arbitrary
programs on the Bifferboard.  Remove these applets if you want any kind of 
security, or use something much more sensible for 2021, e.g. ESP32, beaglebone
or whatever :-D.

