#!/bin/sh

echo "Content-type: text/html; charset=iso-8859-1"
echo

echo "<html>"
echo "<head>"
echo "<meta http-equiv='refresh' content='2'\>"
echo "</head>"
echo "<body>"
echo "Current temperature: "
cat /var/ch_temperature.txt
echo "<br>"
echo "Thermostat setting: "
cat /var/ch_thermostat.txt
echo "<br>"

echo "</body>"
echo "</html>"

