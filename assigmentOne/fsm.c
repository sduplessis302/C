/*
 * fsm_mode2.c
 *
 *  Created on: 30/08/2019
 *      Author: sdup751
 */

#include "fsm.h"
#include <stdio.h>
#include <system.h>
#include <altera_avalon_pio_regs.h> // to use PIO functions
#include <alt_types.h> // alt_u32 is a kind of alt_types
#include <sys/alt_irq.h>

//Enums
enum mode{s_init, s_mode1, s_mode2, s_mode3, s_mode4};
enum mode currentMode;

//UART print char arrays
char printCameraActivated[30] = "Camera Activated";
char printSnapshotTaken[30] = "Snapshot Taken";
char printVehicleEntering[30] = "Vehicle Entering";
char printVehicleLeaving[30] = "Vehicle Leaving";

//Inputs
int sw17;
int switchesValue;
FILE *uart;

//Flags
int printST;
int printVL;
int out_red = 1;
int badInput = 1;
int timeoutCar = 0;
int timeoutMS = 0;
int tempRed = 0;
int allowCrossing = 0;
int blockTimer = 0;

//Time variables
alt_u32 NS_G_time = 6000;
alt_u32 NS_Y_time = 2000;
alt_u32 NS_R_time = 500;
alt_u32 EW_G_time = 6000;
alt_u32 EW_Y_time = 2000;
alt_u32 EW_R_time = 500;

//Storage
int input[6] = {10000, 10000, 10000, 10000, 10000, 10000};
char tempC;
int tempI;
int index;


//FSM Tick
void mode2_tick(){

	sw17 = (switchesValue&(0b100000000000000000)) != 0; //Get switch values for timer input

	updateState();
	cameraFunctionality();
	updateOutputs();
	getUARTData();
	updateTimerValues();
	tempRed = out_red;
}



void updateTimerValues() {
	//Update values if valid input has been provided
	if (badInput == 0 && (currentMode == s_mode3 || currentMode == s_mode4)) {
		NS_G_time = input[0];
		NS_Y_time = input[1];
		NS_R_time = input[2];

		EW_G_time = input[3];
		EW_Y_time = input[4];
		EW_R_time = input[5];
		badInput = 1;

	//Default values
	} else if (currentMode == s_mode2 || currentMode == s_mode1) {
		NS_G_time = 6000;
		NS_Y_time = 2000;
		NS_R_time = 500;

		EW_G_time = 6000;
		EW_Y_time = 2000;
		EW_R_time = 500;
	} else {
		return;
	}
}

//Update state, reset timeout flag & start timer
void updateState(){
	if(timeout == 1){
		//RR
		if (currentState == s_1){
			nextState = s_2;
			timeout = 0;
			alt_alarm_start(&timer, EW_G_time, timer_isr_function, timerContext);

		//GR
		} else if(currentState == s_2){
			nextState = s_3;
			timeout = 0;
			alt_alarm_start(&timer, EW_Y_time, timer_isr_function, timerContext);

		//YR
		} else if(currentState == s_3){
			nextState = s_4;
			timeout = 0;
			alt_alarm_start(&timer, EW_R_time, timer_isr_function, timerContext);

		//RR
		} else if(currentState == s_4){
			nextState = s_5;
			timeout = 0;
			alt_alarm_start(&timer, NS_G_time, timer_isr_function, timerContext);

		//RG
		} else if(currentState == s_5){
			nextState = s_6;
			timeout = 0;
			alt_alarm_start(&timer, NS_Y_time, timer_isr_function, timerContext);

		//RY
		} else {
			nextState = s_1;
			timeout = 0;
			alt_alarm_start(&timer, NS_R_time, timer_isr_function, timerContext);
		}
		currentState = nextState;

	}
}

//Sets light outputs & out_red (Signals red/red state)
void updateOutputs(){

	//RR
	if (currentState == s_1){
			NS_G = 0;
			NS_Y = 0;
			NS_R = 1;

			EW_G = 0;
			EW_Y = 0;
			EW_R = 1;

			NS_ped = 0;
			EW_ped = 0;

			//Set allowCrossing flag if button pushed
			if (buttonOneFlag == 1) {
				allowCrossing = 1;
			}

			out_red = 1;

		//GR
		} else if(currentState == s_2){
			NS_G = 1;
			NS_Y = 0;
			NS_R = 0;

			EW_G = 0;
			EW_Y = 0;
			EW_R = 1;

			//Pedestrian crossing
			if (buttonOneFlag == 1 && currentMode != s_mode1 && allowCrossing == 1) {
				NS_ped = 1;
				buttonOneFlag = 0;
				allowCrossing = 0;
			} else if (currentMode == s_mode1 ){ //Disabled in mode1
				NS_ped = 0;
				buttonOneFlag = 0;
				allowCrossing = 0;
			}
			EW_ped = 0;

			out_red = 0;

		//YR
		} else if(currentState == s_3){
			NS_G = 0;
			NS_Y = 1;
			NS_R = 0;

			EW_G = 0;
			EW_Y = 0;
			EW_R = 1;

			NS_ped = 0;
			EW_ped = 0;

			out_red = 0;

		//RR
		} else if(currentState == s_4){
			NS_G = 0;
			NS_Y = 0;
			NS_R = 1;

			EW_G = 0;
			EW_Y = 0;
			EW_R = 1;

			NS_ped = 0;
			EW_ped = 0;

			//Set allowCrossing flag if button pushed
			if (buttonTwoFlag == 1) {
				allowCrossing = 1;
			}

			out_red = 1;

		//RG
		} else if(currentState == s_5){
			NS_G = 0;
			NS_Y = 0;
			NS_R = 1;

			EW_G = 1;
			EW_Y = 0;
			EW_R = 0;

			//Pedestrian crossing
			if (buttonTwoFlag == 1 && currentMode != s_mode1 && allowCrossing == 1) {
				EW_ped = 1;
				buttonTwoFlag = 0;
				allowCrossing = 0;
			} else if (currentMode == s_mode1){ //Disabled in mode 1
				EW_ped = 0;
				buttonTwoFlag = 0;
				allowCrossing = 0;
			}
			NS_ped = 0;

			out_red = 0;

		//RY
		} else {
			NS_G = 0;
			NS_Y = 0;
			NS_R = 1;

			EW_G = 0;
			EW_Y = 1;
			EW_R = 0;

			NS_ped = 0;
			EW_ped = 0;

			out_red = 0;
		}
}


void cameraFunctionality() {
	if (currentMode == s_mode4) { //Only used in mode 4

		//Starts timer to time how long vehicle in intersection
		if (buttonThreeFlag == 1 && blockTimer == 0) {
			alt_alarm_start(&ms, 1, ms_isr_function, NULL);
			blockTimer = 1;
			writeToUART(printVehicleEntering, NULL);
		}

		//Starts 2s timer in Yellow state
		if (buttonThreeFlag == 1 && (currentState == s_3 || currentState == s_6) && timeoutCar == 0) {
			alt_alarm_start(&carTimer, 2000, camera_isr_function, timerContext);
			timeoutMS = 0;
			timeoutCar = 1;
			printf("Camera activated\n");
			writeToUART(printCameraActivated,NULL);
		}

		//Prints snaphot taken if needed
		if (printST == 1) {
			writeToUART(printSnapshotTaken,NULL);
			printST = 0;
		}

		//Prints Vehicle Leaving & time
		if (printVL == 1) {
			alt_alarm_stop(&ms);
			writeToUART(printVehicleLeaving,NULL);
			writeToUART(NULL,timeCounted);
			blockTimer = 0;
			timeCounted = 0; //Reset time
			printVL = 0;
		}

	}
}

//User input via UART
void getUARTData(){

	//Prepare to receive input
	if (sw17 == 1 && tempRed == 1 && (currentMode == s_mode3 || currentMode == s_mode4)) {

		//Wait for switch to be moved down
		printf("Move SW17 to down position before entering input\n");
		while(sw17 == 1){
			switchesValue = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
			sw17 = (switchesValue&(0b100000000000000000)) != 0;
		}
		printf("Waiting for input...\n");

		//Open UART and reset variables
		uart = fopen(UART_NAME, "r");
		tempI = 0;
		index = 0;
		badInput = 0;

		//Check uart opened and get first character
		if (uart!= NULL){
			tempC = fgetc(uart);
		} else{
			badInput = 1;
		}

		//While character is not \n
		while(tempC!= '\n'){

			//Check for important characters
			if(tempC == ',' || tempC == '\r' || tempC == '\n'){

				//Reject if <5 digits or negative
				if (tempI > 9999 || tempI <= 0){
					badInput = 1;
				}

				//Store the number and reset temp
				input[index] = tempI;
				index++;
				tempI = 0;

				//Break if last char
				if(tempC == '\r' || tempC == '\n'){
					break;
				}

			//Reject input if invalid character received
			} else if((tempC <= 47 || tempC >= 58)){
				badInput = 1;
			} else{
				tempI = (tempI * 10) + (tempC - 48); //Left shift digit & add next digit
			}

			tempC = fgetc(uart); //Get next char


		}

		//Reject input if more/less than 6 numbers received
		if(index != 6){
			badInput = 1;
		}

		//Print input to console
		for(int i = 0; i < 6; i++){
			printf("%d, ",input[i]);
		}
		printf("\n");

		if(badInput == 1){
			printf("Input Rejected\n");
		}else{
			printf("Input Accepted\n");
		}

		//Reset var and close uart
		tempRed = 0;
		fclose(uart);
	}


}


