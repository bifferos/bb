Scripts have been added here to allow easier generation of firmware for 1MB
flash devices.  The firmware is designed to be extensible, and under the
control of a coordinating machine somewhere on the network, which has the job
of augmenting the basic boot firmware with an 'application' uploaded over
ethernet.

To compile the toolchain:
./mkbiffrd.py bootstrap

