2.6.37.6 Kernel 

This is a standard kernel with a number of alterations to suit running under 
memory constrained systems:

Flashing
========

The kernel contains two drivers for flashing new firmware,
/dev/biffboot - cat the new boot image into this file to write a new
bootloader.
/dev/biffkernel - cat the new kernel image into this file to write a new 
kernel.

The in-kernel drivers for flashing save space over userspace nor flash 
utils.

x86 support
===========

Much of the x86 cpu support code cannot be removed from the kernel.
For the Bifferboard support for any CPU features above 486 is simply
wasted space.  Many architectures have been removed.

Math EMU
========

Math emulation code is pointless on the Bifferboard.  Anything that requires
FPU support is going to run so insanely slow that it's not worth using. 
mjpeg UVC cameras are fine, but if you have to resort to image decompression
expect 1fps.  Normally the kernel errors if booting without FPU support and
no compiled in emulator, but this has been worked around to save about 10k
of space.

