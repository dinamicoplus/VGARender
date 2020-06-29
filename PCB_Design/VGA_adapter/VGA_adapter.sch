EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr User 11102 7087
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
P 5000 4850
F 0 "J1" H 5730 4896 50  0000 L CNN
F 1 "VGA Output" H 5730 4805 50  0000 L CNN
F 2 "Connector_Dsub:DSUB-15-HD_Female_Horizontal_P2.29x1.98mm_EdgePinOffset3.03mm_Housed_MountingHolesOffset4.94mm" H 4050 5250 50  0001 C CNN
F 3 " ~" H 4050 5250 50  0001 C CNN
	1    5000 4850
	1    0    0    -1  
$EndComp
$Comp
L custom:Conn_02x28_Counter_Clockwise J2
U 1 1 5EB69B1A
P 5050 2550
F 0 "J2" H 5025 3375 50  0000 C CNN
F 1 "PIC24 Curiosity Board Adapter" H 5025 3284 50  0000 C CNN
F 2 "custom:2xPinHeader_2x14_P2.54mm_Vertical" H 5100 1625 50  0001 C CNN
F 3 "~" H 5100 1625 50  0001 C CNN
	1    5050 2550
	1    0    0    -1  
$EndComp
Text GLabel 3500 1950 0    50   Input ~ 0
GND
Text GLabel 3500 2850 0    50   Input ~ 0
GND
Text GLabel 3500 3250 0    50   Input ~ 0
GND
Text GLabel 4800 2450 2    50   Input ~ 0
GND
Text GLabel 5300 1950 0    50   Input ~ 0
GND
Text GLabel 7000 2350 2    50   Input ~ 0
GND
Text GLabel 6500 2850 2    50   Input ~ 0
GND
Text GLabel 5500 4100 1    50   Input ~ 0
GND
Text GLabel 5300 4100 1    50   Input ~ 0
GND
Text GLabel 5100 4100 1    50   Input ~ 0
GND
Text GLabel 4700 4100 1    50   Input ~ 0
GND
Wire Wire Line
	4600 2450 4800 2450
Wire Wire Line
	5300 1950 5500 1950
Text GLabel 5400 4100 1    50   Input ~ 0
RED
Text GLabel 5200 4100 1    50   Input ~ 0
GREEN
Text GLabel 5000 4100 1    50   Input ~ 0
BLUE
Text GLabel 5000 5650 3    50   Input ~ 0
H_SYNC
Text GLabel 4800 5650 3    50   Input ~ 0
V_SYNC
Wire Wire Line
	3500 1950 4100 1950
Wire Wire Line
	3500 2850 4100 2850
Wire Wire Line
	3500 3250 4100 3250
Wire Wire Line
	6500 2350 7000 2350
Wire Wire Line
	6000 2850 6500 2850
Wire Wire Line
	5500 4100 5500 4550
Wire Wire Line
	5400 4100 5400 4550
Wire Wire Line
	5300 4100 5300 4550
Wire Wire Line
	5200 4100 5200 4550
Wire Wire Line
	5100 4100 5100 4550
Wire Wire Line
	5000 4100 5000 4550
Wire Wire Line
	4700 4100 4700 4550
Wire Wire Line
	5000 5150 5000 5650
Wire Wire Line
	4800 5150 4800 5650
Text GLabel 3500 2150 0    50   Input ~ 0
H_SYNC
Wire Wire Line
	3500 2150 4100 2150
Text GLabel 4850 3650 0    50   Input ~ 0
RED
Wire Wire Line
	4850 3650 4950 3650
Wire Wire Line
	5350 3250 5500 3250
Text GLabel 7000 3400 2    50   Input ~ 0
GREEN
Wire Wire Line
	6500 3400 6150 3400
Wire Wire Line
	6150 3400 6150 3250
Wire Wire Line
	6150 3250 6000 3250
Text GLabel 7000 2650 2    50   Input ~ 0
BLUE
Wire Wire Line
	6500 2650 6000 2650
Text GLabel 3500 2250 0    50   Input ~ 0
V_SYNC
Wire Wire Line
	3500 2250 4100 2250
NoConn ~ 4100 2050
NoConn ~ 4100 2350
NoConn ~ 4100 2450
NoConn ~ 4100 2550
NoConn ~ 4100 2650
NoConn ~ 4100 2750
NoConn ~ 4100 2950
NoConn ~ 4100 3050
NoConn ~ 4100 3150
NoConn ~ 4600 3250
NoConn ~ 4600 3150
NoConn ~ 4600 3050
NoConn ~ 4600 2950
NoConn ~ 4600 2850
NoConn ~ 4600 2750
NoConn ~ 4600 2650
NoConn ~ 4600 2550
NoConn ~ 4600 2350
NoConn ~ 4600 2250
NoConn ~ 4600 2150
NoConn ~ 4600 2050
NoConn ~ 4600 1950
NoConn ~ 5500 2050
NoConn ~ 5500 2150
NoConn ~ 5500 2250
NoConn ~ 5500 2350
NoConn ~ 5500 2450
NoConn ~ 5500 2550
NoConn ~ 5500 2650
NoConn ~ 5500 2750
NoConn ~ 5500 2850
NoConn ~ 5500 2950
NoConn ~ 5500 3050
NoConn ~ 5500 3150
NoConn ~ 6000 3150
NoConn ~ 6000 3050
NoConn ~ 6000 2950
NoConn ~ 6000 2750
NoConn ~ 6000 2550
NoConn ~ 6000 2350
NoConn ~ 6000 2250
NoConn ~ 6000 2150
NoConn ~ 6000 2050
NoConn ~ 6000 1950
NoConn ~ 4900 4550
NoConn ~ 4800 4550
NoConn ~ 4600 5150
NoConn ~ 4600 4550
NoConn ~ 5200 5150
NoConn ~ 5400 5150
$Comp
L Device:R R2
U 1 1 5EF7B548
P 6650 2650
F 0 "R2" V 6443 2650 50  0000 C CNN
F 1 "470" V 6534 2650 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 6580 2650 50  0001 C CNN
F 3 "~" H 6650 2650 50  0001 C CNN
	1    6650 2650
	0    1    1    0   
$EndComp
$Comp
L Device:R R3
U 1 1 5EF7BB41
P 6650 3400
F 0 "R3" V 6800 3350 50  0000 L CNN
F 1 "470" V 6720 3355 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 6580 3400 50  0001 C CNN
F 3 "~" H 6650 3400 50  0001 C CNN
	1    6650 3400
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R1
U 1 1 5EF7C229
P 5100 3650
F 0 "R1" V 5307 3650 50  0000 C CNN
F 1 "470" V 5216 3650 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric" V 5030 3650 50  0001 C CNN
F 3 "~" H 5100 3650 50  0001 C CNN
	1    5100 3650
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6000 2450 6500 2450
Wire Wire Line
	6500 2450 6500 2350
Wire Wire Line
	6800 2650 7000 2650
Wire Wire Line
	5250 3650 5350 3650
Wire Wire Line
	5350 3250 5350 3650
Wire Wire Line
	6800 3400 7000 3400
$EndSCHEMATC
