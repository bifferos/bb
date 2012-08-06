Scripts have been added here to allow easier generation of firmware for 1MB
flash devices.  The firmware is designed to be extensible, and under the
control of a coordinating machine somewhere on the network, which has the job
of augmenting the basic boot firmware with an 'application' uploaded over
ethernet.

Quickstart:
./mkbiffrd.py toolchain
./mkbiffrd.py compile

- Flash the file ./bzImage to the board
- Set kernel command-line to console=uart,io,0x3f8 biffip=<ipaddress>
NB:  IP autoconfiguration (DHCP) is not supported.

- Put any openwrt packages (.ipk files) in the 'packages' directory and they
will be put in a tarball called 'ftp.tgz' when you run the 'upload' command:

./mkbiffrd.py upload <ipaddress>

The above command extracts the data.tar.gz parts of the opkg, puts them
all in a single tarball along with any built kernel modules and uploads them to
the board via ftp.


Application Example
===================

Suppose you want to setup a system with 1MB flash to run mjpg-streamer. This
is impossible in 1MB of RAM because mjpg-streamer cannot be statically
linked.  In any case it's a struggle to link in all necessary kernel modules
in 1MB of flash.


Step 1 - Compile kernel modules

run ./mkbiffrd.py /config-kernel

Choose USB modules and any required webcam modules.  Save and quit.

run ./mkbiffrd.py compile


Step 2 - Grab mjpg-streamer and deps from an OpenWrt build.

You will need the following packages:

libc_0.9.32-79_rdc.ipk
libgcc_4.5-linaro-79_rdc.ipk
libjpeg_6b-1_rdc.ipk
libpthread_0.9.32-79_rdc.ipk
mjpg-streamer_r136-1_rdc.ipk

You may also need some files in a /www directory for mjpg-streamer,
depending on the command-line used.


Step 3 - Experiment by 'pushing' a tarball with the files to the board

run ./mkbiffrd.py upload <addr>

Where <addr> is the address of the board.  This will generate a local copy
of the rootfs (ftp.tgz) and then upload it to /ftp.tgz.

Then telnet to the board and
extract the tarball:

tar xf ftp.tgz


Step 4 - Switch to automated download of your application

Make ftp.tgz available on a webserver on your network, perhaps even another
Bifferboard.
Add some instructions to the end of files/etc/init.d/rcS, something like:

cd /
wget http://<server>/ftp.tgz
tar xf ftp.tgz
mjpg-streamer <args> &

Now re-compile your kernel:
./mkbiffrd.py compile

And flash the image.



