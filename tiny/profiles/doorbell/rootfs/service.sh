#!/bin/sh

echo "Starting doorbell monitor service"

while true
do
    BUTTON_VALUE="$(cat /sys/class/gpio/gpio15/value)"
    if [ "$BUTTON_VALUE" == "0" ]; then
       /buzz.sh
       usleep 9000
    fi
    usleep 1000
done
