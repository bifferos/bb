

.PHONY: all run vde_start vde_stop qemu_debug image


# Build a firmware
all:
	make -C openwrt
	cd qemu && ./configure
	make -C qemu
	make -C seabios


# Run the emulation
run:
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

# Add options for usb bus and devices if needed
#		-usb  -usbdevice disk:usbdisk.img


# Setup the VDE switch
vde_start:
	sudo /sbin/modprobe tun
	sudo vde_switch -hub -tap tap0 -daemon
	sudo chmod -R a+rwx /var/run/vde.ctl
	sudo vde_pcapplug eth0 -daemon


# Close the VDE switch
vde_stop:
	sudo killall vde_pcapplug
	sudo killall vde_switch


# Startup gdb with options to debug qemu
qemu_debug:
	gdb qemu/i386-softmmu/qemu -pid `ps -C qemu -o pid=` -x qemu-debug-startup.scr


# Make a test image
image:
	./qemu/qemu-img create usbdisk.img 4G




