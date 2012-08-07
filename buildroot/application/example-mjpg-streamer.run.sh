#!/bin/sh
# Example application script for video streaming
# Copy this to run.sh

# Only needed if you installed modules
depmod
# Access to USB
modprobe ohci-hcd
modprobe ehci-hcd
# Access to webcams
modprobe uvcvideo
# Update device nodes
mdev -s
# Actually run the application software/start daemons.
mjpg_streamer -i "input_uvc.so -y -r 320x240" -o "output_http.so -w /www -p 8090"
