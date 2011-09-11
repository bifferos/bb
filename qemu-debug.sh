#!/bin/sh

gdb qemu/i386-softmmu/qemu -pid `ps -C qemu -o pid=` -x qemu-debug-startup.scr





