#!/bin/sh

echo "Starting service"

while true
do
    nc -l -p 45567 -u -n -s 0.0.0.0 -e /buzz.sh
done
