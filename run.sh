#!/bin/sh

#
# Run the built Openwrt image with the built qemu
#
# Uses the BIOS from qemu, it's far more appropriate for setting up 
# qemu than a hacked Biffboot!



./qemu/i386-softmmu/qemu \
	-cpu 486 -m 32  \
	-bios seabios/out/bios.bin  \
	-firmware openwrt/bin/rdc/openwrt-rdc-jffs2-64k-bifferboard.img   \
	-kernel openwrt/bin/rdc/openwrt-rdc.bzImage   \
	-append "console=uart,io,0x3f8 rootfstype=jffs2"   \
	-vga none -nographic \
	-L qemu/pc-bios/optionrom  \
	-net nic,model=r6040,macaddr=52:54:00:12:34:57   \
	-net user


