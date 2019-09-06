Traffic Lights Embedded System - multiple modes - configurable via button 
inputs/UART

################################################################################
/*									      */
/*		rgup198 				sdup751		      */
/*		     		      CS303				      */
/*		                   Assignment 1			              */
/*									      */
################################################################################

Files included:
 - main.c
 - fsm.c
 - fsm.h
 - sof file
 - sopcinfo file

Mode Selection:
	SW_1 | SW_0 | Mode
	 0   |   0  |  1
	 0   |   1  |  2
	 1   |   0  |  3
	 1   |   1  |  4

Mode 1:
	No user input

Mode 2:
	KEY_0 - NS Pedestrian button
	KEY_1 - EW Pedestrian button

Mode 3:
	KEY_0 - NS Pedestrian button
	KEY_1 - EW Pedestrian button
	SW_17 - Configure times (follow console instructions, send via UART)

Mode 4:
	KEY_0 - NS Pedestrian button
	KEY_1 - EW Pedestrian button
	KEY_2 - Vehicle entering/leaving
	SW_17 - Configure times (follow console instructions, send via UART)


################################################################################
SW_17 Usage:
	Move switch up till TLC reaches RR state, move switch down.
	Enter data in PUTTY terminal like so:
		xxxx,xxxx,xxxx,xxxx,xxxx,xxxx[\r]\n

################################################################################
