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
- Once the build has completed, transfer it over to your main 64-bit desktop
  machine and the compiler in ./output/host/usr/bin/i486-unknown-linux-uclibc-*
  should run.
- Most compiler files just require glibc so will be happy.
- cc1 depends on mpfr, so make a link from mpfr.so.4 -> mpfr.so.6 in
  /usr/lib64
- Put the built toolchain in ../../buildroot-2011/11/output/host


