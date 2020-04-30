Directories under this level contain the files needed to customise
the rootfs for a particular application.

For instance, if you want to build the 'buzzer' application, run:

./mkbiffrd.py compile buzzer

This will take whatever busybox and kernel have been configured, and
add the buzzer rootfs files to the standard busybox rootfs.

Obviously you'll need to include whatever kernel modules are needed
with ./mkbiffrd.py config first.

