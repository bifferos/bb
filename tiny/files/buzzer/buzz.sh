#!/bin/sh

#  Sound the buzzer 

buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1
sleep 1
buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1
sleep 1
buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1
sleep 1
buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1
sleep 1
buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1
sleep 1
buzzer -t 1000 /dev/ttyUSB0 TIOCM_RTS 1



