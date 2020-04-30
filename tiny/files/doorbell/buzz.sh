#!/bin/sh

nc -u -n 255.255.255.255 999 -e echo buzz $1 $2
