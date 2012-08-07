Scripts have been added here to allow easier generation of firmware for 1MB
flash devices.  The firmware is designed to be extensible, and under the
control of a coordinating machine somewhere on the network, which has the job
of augmenting the basic boot firmware with an 'application' uploaded over
ethernet.

Quickstart
==========

./mkbiffrd.py toolchain
./mkbiffrd.py compile

- Flash the file ./bzImage to the board
- Set kernel command-line to console=uart,io,0x3f8 biffip=<ipaddress>
NB:  IP autoconfiguration (DHCP) is not supported.

This gives you a system with telnet daemon (login with no password, it's
totally insecure), ftp daemon and basic module loading utilities. Drop
scripts or binaries into the files directory to augment this basic image
with extra functionality, and then recompile the bzImage with:

./mkbiffrd.py compile

If you need to add extra kernel drivers run:

./mkbiffrd.py config-kernel

If you need to add additional busybox applets (assuming they fit in 1MB) then
run:

./mkbiffrd.py config-busybox

Adding applets will automatically generate extra file system links to the
busybox binary but you may also need to add config files to files/etc.


When you run out of space
=========================

Additional functionality must now be uploaded via ethernet.  It can either
be 'pushed' by uploading an ftp tarball and logging in to the system to
decompress it and run programs, or it can be pulled by changing the init
script behaviour.  Init script behaviour is changed via the Biffboot kernel
command-line, so no changes to the boot image are required.

It's best to start off by doing things manually:

- Put any required openwrt packages (.ipk files) in the 'packages' directory.
- Configure the kernel as above adding any required drivers _as_modules_. 
  Don't compile them into the kernel!
- Edit application/run.sh to make it do what you want on boot, e.g. start
  daemons, load modules etc...  You may want to add a mdev -a after loading
  device drivers to ensure dev files are created under /dev.

run :
./mkbiffrd.py makeapp 

to create the tarball.

This will create approot.tgz, which you can ftp to the booted board.  Telnet
to the board, decompress /approot.tgz (must be decompressed into the root),
and then run your run.sh script with /run.sh.

When everything is working OK, you can automate the above.


Automating the process
======================

The init script also supports a biffserver=<addr> kernel command line.  If not
set, nothing will happen and the system will boot normally.  If set, the
init scripts will attempt to download a tarball from the following URL:

http://<biffserver>/applications/<biffip>/approot.tgz

E.g. with bifferboard command-line:
console=uart,io,0x3f8 biffip=192.168.0.90 biffserver=192.168.0.2

This will try to download
http://192.168.0.2/applications/192.168.0.90/approot.tgz

The tarball will be extracted into root and the init script will then attempt to run
the shell script found at /run.sh.


Application Example
===================

Suppose you want to setup a system with 1MB flash to run mjpg-streamer. This
is impossible in 1MB of RAM because mjpg-streamer cannot be statically
linked.  In any case it's a struggle to link in all necessary kernel modules
in 1MB of flash.


Step 1 - Compile kernel modules

run ./mkbiffrd.py config-kernel

Choose USB modules and any required video modules.  Add them as [m].  Save and quit.

run ./mkbiffrd.py compile


Step 2 - Grab mjpg-streamer and deps from an OpenWrt build.

You will need the following packages:

libc_0.9.32-79_rdc.ipk
libgcc_4.5-linaro-79_rdc.ipk
libjpeg_6b-1_rdc.ipk
libpthread_0.9.32-79_rdc.ipk
mjpg-streamer_r136-1_rdc.ipk

Copy them to ./packages/.

You may also need some files in a /www directory for mjpg-streamer,
depending on the command-line used.  They can be either added to the ./files
directory, and risk bloating the firmware, or you can simply make up a fake
'package' for them, by putting them in a tarball called './data.tar.gz' and
then wrapping that tarball in another one called something like
'mjpg-streamer-www_1_rdc.ipk'.  The files will get extracted along with the
other packages.

Step 3 - Construct your run.sh script

For this application check out the commented example script in the
application directory.


Step 4 - Experiment by 'pushing' a tarball with the files to the board

run ./mkbiffrd.py makeapp

then upload approot.tgz to /approot.tgz on the board using ftp.

Then telnet to the board and
extract the tarball:

tar xf approot.tgz

You should be able to run /run.sh at this point and see something
happening on the webcam.


Step 5 - Switch to automated download of your application

Make approot.tgz available on a webserver on your network, perhaps even another
Bifferboard (one with JFFS).  See the URL description above.  Set biffserver
kernel command-line from Biffboot to your server IP address and reboot the
board.




