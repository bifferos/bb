EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Isolator:CNY17-3 U1
U 1 1 61338315
P 4850 2950
F 0 "U1" H 4850 3275 50  0000 C CNN
F 1 "CNY17-3" H 4850 3184 50  0000 C CNN
F 2 "" H 4850 2950 50  0001 L CNN
F 3 "http://www.vishay.com/docs/83606/cny17.pdf" H 4850 2950 50  0001 L CNN
	1    4850 2950
	1    0    0    -1  
$EndComp
$Comp
L Device:R R1
U 1 1 613397A7
P 4500 2550
F 0 "R1" H 4570 2596 50  0000 L CNN
F 1 "R100" H 4570 2505 50  0000 L CNN
F 2 "" V 4430 2550 50  0001 C CNN
F 3 "~" H 4500 2550 50  0001 C CNN
	1    4500 2550
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x04_Male J1
U 1 1 6133FA07
P 3750 2950
F 0 "J1" H 3858 3231 50  0000 C CNN
F 1 "Edge" H 3858 3140 50  0000 C CNN
F 2 "" H 3750 2950 50  0001 C CNN
F 3 "~" H 3750 2950 50  0001 C CNN
	1    3750 2950
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x03_Male J2
U 1 1 6134BA1D
P 5800 2850
F 0 "J2" H 5772 2874 50  0000 R CNN
F 1 "PCB header" H 5772 2783 50  0000 R CNN
F 2 "" H 5800 2850 50  0001 C CNN
F 3 "~" H 5800 2850 50  0001 C CNN
	1    5800 2850
	-1   0    0    -1  
$EndComp
Wire Wire Line
	3950 2850 3950 2400
Wire Wire Line
	3950 2400 4500 2400
Wire Wire Line
	3950 3050 4550 3050
Wire Wire Line
	4500 2700 4500 2850
Wire Wire Line
	4500 2850 4550 2850
Wire Wire Line
	5600 2750 5600 2400
Wire Wire Line
	5600 2400 4500 2400
Connection ~ 4500 2400
Wire Wire Line
	5600 2950 5150 2950
Wire Wire Line
	3950 3150 5150 3150
Wire Wire Line
	5150 3150 5150 3050
$EndSCHEMATC
