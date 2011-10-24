

.PHONY: all run vde_start vde_stop qemu_debug image ser_upload eth_upload

QEMU_BIN = ./qemu/i386-softmmu/qemu
BIOS_BIN = ./bios.bin
OPENWRT_ROOTFS = openwrt/build_dir/linux-rdc/root.jffs2-64k
OPENWRT_FIRMWARE = openwrt/bin/rdc/openwrt-rdc-jffs2-64k-bifferboard.img
QEMU_FIRMWARE = qemu-firmware.img

# Build a firmware
all:
	make -C openwrt
	cd qemu && ./configure
	make -C qemu
	make -C seabios

$(QEMU_BIN):
	cd qemu && ./configure
	make -C qemu

$(SEABIOS_BIN):
	make -C seabios

$(OPENWRT_ROOTFS) $(OPENWRT_FIRMWARE):
	make -C openwrt

# no dependencies, to prevent a customised Qemu firmware being overwritten
$(QEMU_FIRMWARE):
	cp $(OPENWRT_FIRMWARE) $@

$(BIOS_BIN):
	wget http://bifferos.co.uk/downloads/bifferboard/qemu/biffboot-3_5-qemu.bin -O $@
#	To use seabios...
#	cp ./seabios/out/bios.bin $@
	

# Run the emulation
run: $(QEMU_BIN) $(BIOS_BIN) $(QEMU_FIRMWARE)
	$(QEMU_BIN) \
		-cpu 486 -m 32 -kmax 0x10  \
		-bios $(BIOS_BIN)  \
		-rtc base="2009-08-07T04:02:00" \
		-firmware $(QEMU_FIRMWARE)   \
		-vga none -nographic \
		-L qemu/pc-bios/optionrom  \
		-net nic,model=r6040,macaddr=52:54:00:12:34:57   \
		-net user


		
		
# Add option for discarding the flash if you don't want to save it each time
#		-discard-flash

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
	gdb $(QEMU_BIN) -pid `ps -C qemu -o pid=` -x qemu/qemu-debug-startup.scr


# Make a test image
image:
	./qemu/qemu-img create usbdisk.img 4G

# Upload to 8MB bifferboard over serial - you may need to change the PC
# serial device
ser_upload: $(OPENWRT_FIRMWARE)
	tools/bb_upload8.py /dev/ttyUSB0 $<

# Upload to 8MB bifferboard over ethernet - you *will* need to change the
# Bifferboard MAC address, and you may need to change the PC network device
eth_upload: $(OPENWRT_FIRMWARE)
	sudo tools/bb_eth_upload8.py eth0 ff:ff:ff:ff:ff:ff $<

