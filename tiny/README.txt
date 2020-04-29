Modifications to busybox and linux that allow really tiny systems on the
Bifferboard, a device with 1MB flash and 32MB DRAM.

Use of Kernel 2.6 requires some extra steps to get a compilation, because
recent compilers cannot compile a kernel this old.

We will use buildroot for the toolchain, however we need to use an old 
version of buildroot to get the correct compiler version to compile the 
Linux kernel.  Using a sufficiently old version of buildroot means we can
no longer compile it on the latest Slackware (or probably any recent Linux).

The following strategy is used:

- Obtain Slackware 13.37 (64-bit version), perform full install in VM.
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

For booting the bifferboard, you should use a command-line such as:
console=uart,io,0x3f8 biffip=172.17.0.15

DHCP kernel support takes up a fair amount of space, so it's better to
configure the IP address via biffip, which is picked up by biffparam.

There isn't space for openssl in 1MB of flash, so busybox runs a telnet
daemon.  There is no security at all, not even a password.  Likewise with
the ftp server.  This allows uploading of files and running arbitrary
programs on the Bifferboard.  Remove these applets if you want any kind of 
security.

