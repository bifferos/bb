#!/bin/sh

nc -u -n 255.255.255.255 45567 -e echo buzz $1 $2
