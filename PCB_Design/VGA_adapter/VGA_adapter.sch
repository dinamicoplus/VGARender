EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "PIC24 VGA Plug-In Adapter"
Date "2020-05-03"
Rev "1"
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L custom:DB15_Female_HighDensity J1
U 1 1 5EBA3C45
P 5400 5250
F 0 "J1" H 6130 5296 50  0000 L CNN
F 1 "VGA Output" H 6130 5205 50  0000 L CNN
F 2 "Connector_Dsub:DSUB-15-HD_Female_Horizontal_P2.29x1.98mm_EdgePinOffset3.03mm_Housed_MountingHolesOffset4.94mm" H 4450 5650 50  0001 C CNN
F 3 " ~" H 4450 5650 50  0001 C CNN
	1    5400 5250
	1    0    0    -1  
$EndComp
$Comp
L custom:Conn_02x28_Counter_Clockwise J2
U 1 1 5EB69B1A
P 5450 3250
F 0 "J2" H 5425 4075 50  0000 C CNN
F 1 "PIC24 Curiosity Board Adapter" H 5425 3984 50  0000 C CNN
F 2 "custom:2xPinHeader_2x14_P2.54mm_Vertical" H 5500 2325 50  0001 C CNN
F 3 "~" H 5500 2325 50  0001 C CNN
	1    5450 3250
	1    0    0    -1  
$EndComp
Text GLabel 3900 2650 0    50   Input ~ 0
GND
Text GLabel 3900 3550 0    50   Input ~ 0
GND
Text GLabel 3900 3950 0    50   Input ~ 0
GND
Text GLabel 5200 3150 2    50   Input ~ 0
GND
Text GLabel 5700 2650 0    50   Input ~ 0
GND
Text GLabel 6900 3150 2    50   Input ~ 0
GND
Text GLabel 6900 3550 2    50   Input ~ 0
GND
Text GLabel 5900 4500 1    50   Input ~ 0
GND
Text GLabel 5700 4500 1    50   Input ~ 0
GND
Text GLabel 5500 4500 1    50   Input ~ 0
GND
Text GLabel 5100 4500 1    50   Input ~ 0
GND
Wire Wire Line
	5000 3150 5200 3150
Wire Wire Line
	5700 2650 5900 2650
Text GLabel 5800 4500 1    50   Input ~ 0
RED
Text GLabel 5600 4500 1    50   Input ~ 0
GREEN
Text GLabel 5400 4500 1    50   Input ~ 0
BLUE
Text GLabel 5400 6050 3    50   Input ~ 0
H_SYNC
Text GLabel 5200 6050 3    50   Input ~ 0
V_SYNC
Wire Wire Line
	3900 2650 4500 2650
Wire Wire Line
	3900 3550 4500 3550
Wire Wire Line
	3900 3950 4500 3950
Wire Wire Line
	6400 3150 6900 3150
Wire Wire Line
	6400 3550 6900 3550
Wire Wire Line
	5900 4500 5900 4950
Wire Wire Line
	5800 4500 5800 4950
Wire Wire Line
	5700 4500 5700 4950
Wire Wire Line
	5600 4500 5600 4950
Wire Wire Line
	5500 4500 5500 4950
Wire Wire Line
	5400 4500 5400 4950
Wire Wire Line
	5100 4500 5100 4950
Wire Wire Line
	5400 5550 5400 6050
Wire Wire Line
	5200 5550 5200 6050
Text GLabel 3900 2850 0    50   Input ~ 0
H_SYNC
Wire Wire Line
	3900 2850 4500 2850
Text GLabel 5650 4050 0    50   Input ~ 0
RED
Wire Wire Line
	5650 4050 5750 4050
Wire Wire Line
	5750 4050 5750 3950
Wire Wire Line
	5750 3950 5900 3950
Text GLabel 6900 4100 2    50   Input ~ 0
GREEN
Wire Wire Line
	6900 4100 6550 4100
Wire Wire Line
	6550 4100 6550 3950
Wire Wire Line
	6550 3950 6400 3950
Text GLabel 6900 3350 2    50   Input ~ 0
BLUE
Wire Wire Line
	6900 3350 6400 3350
Text GLabel 3900 2950 0    50   Input ~ 0
V_SYNC
Wire Wire Line
	3900 2950 4500 2950
NoConn ~ 4500 2750
NoConn ~ 4500 3050
NoConn ~ 4500 3150
NoConn ~ 4500 3250
NoConn ~ 4500 3350
NoConn ~ 4500 3450
NoConn ~ 4500 3650
NoConn ~ 4500 3750
NoConn ~ 4500 3850
NoConn ~ 5000 3950
NoConn ~ 5000 3850
NoConn ~ 5000 3750
NoConn ~ 5000 3650
NoConn ~ 5000 3550
NoConn ~ 5000 3450
NoConn ~ 5000 3350
NoConn ~ 5000 3250
NoConn ~ 5000 3050
NoConn ~ 5000 2950
NoConn ~ 5000 2850
NoConn ~ 5000 2750
NoConn ~ 5000 2650
NoConn ~ 5900 2750
NoConn ~ 5900 2850
NoConn ~ 5900 2950
NoConn ~ 5900 3050
NoConn ~ 5900 3150
NoConn ~ 5900 3250
NoConn ~ 5900 3350
NoConn ~ 5900 3450
NoConn ~ 5900 3550
NoConn ~ 5900 3650
NoConn ~ 5900 3750
NoConn ~ 5900 3850
NoConn ~ 6400 3850
NoConn ~ 6400 3750
NoConn ~ 6400 3650
NoConn ~ 6400 3450
NoConn ~ 6400 3250
NoConn ~ 6400 3050
NoConn ~ 6400 2950
NoConn ~ 6400 2850
NoConn ~ 6400 2750
NoConn ~ 6400 2650
NoConn ~ 5300 4950
NoConn ~ 5200 4950
NoConn ~ 5000 5550
NoConn ~ 5000 4950
NoConn ~ 5600 5550
NoConn ~ 5800 5550
$EndSCHEMATC
