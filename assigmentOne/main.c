/*
 * main.c
 *
 *  Created on: 30/08/2019
 *      Author: sdup751
 */
#include <stdio.h>
#include <stdbool.h>
#include "fsm.h"

#include <system.h>
#include <altera_avalon_pio_regs.h>
#include <sys/alt_irq.h>

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

//Enums
enum mode{s_init, s_mode1, s_mode2, s_mode3, s_mode4};
enum mode currentMode;
enum mode nextMode;

//Timers
alt_alarm timer;
void* timerContext;

//pedestrian button input holders
volatile int buttonOneFlag = 0;
volatile int buttonTwoFlag = 0;
volatile int buttonThreeFlag = 0;

//Inputs
int sw0;
int sw1;
int switchesValue;
int buttonsValue;

//Ouputs
int led_out;
FILE *uartPrint;
FILE *lcd;

//Flags
int printST = 0;
int printVL = 0;
int allowNextMode = 1;
int timeCountMain = 0;
int timerStarted = 0;
int vehicleCaught = 1;


int main()
{
	//Initialisation
	currentMode = s_init;
	nextMode = s_init;
	init();
	init_button_interrupts(); // button interupts

	lcd = fopen(LCD_NAME, "w");
	uartPrint = fopen(UART_NAME, "w");


	//Forever
	while(1){
		//Get switches input
		switchesValue = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
		sw0 = (switchesValue&(0b0001)) == 0;
		sw1 = (switchesValue&(0b0010)) == 0;

		//Select next mode if in R/R state
		if (allowNextMode == 1) {
			if (!sw0 && !sw1){
				nextMode = s_mode4;
			} else if(sw0 && !sw1){
				nextMode = s_mode3;
			} else if(!sw0 && sw1){
				nextMode = s_mode2;
			} else{
				nextMode = s_mode1;
			}
			allowNextMode = 0;
		}

		// only allow change of modes while in R,R states
		if (out_red == 1) {
			allowNextMode = 1;
			out_red = 0;
		}

		if(currentMode != nextMode){

			//Print to LCD
			if (lcd != NULL) {
						fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
						fprintf(lcd, "MODE: %d\n", nextMode);
					}
		}

		//Move to next mode
		currentMode = nextMode;
		mode2_tick(); //Tick fsm

		//Assign outputs
		led_out = NS_G + (NS_Y << 1) + (NS_R << 2) + (EW_G << 3) + (EW_Y << 4) + (EW_R << 5) + (NS_ped << 6) + (EW_ped << 7);
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, led_out);
		usleep(100000);

	}


	//Close LCD & UART
	fclose(lcd);
	fclose(uartPrint);

  return 0;
}

//Initialises states & outputs, starts timer
void init(){

	//Set lights to R,R
	NS_G = 0;
	NS_Y = 0;
	NS_R = 1;

	EW_G = 0;
	EW_Y = 0;
	EW_R = 1;

	timerContext = (void*) &timeCountMain;
	alt_alarm_start(&timer, 500, timer_isr_function, timerContext);

	currentState = s_1;
	nextState = s_1;

	printf("Initialised\n");
}


//Initalise interrupts for push buttons
void init_button_interrupts(){

	 int buttonValue = 1;
	 void* context_going_to_be_passed = (void*) &buttonValue;
	 // cast before passing to ISR

	 // clears the edge capture register
	 IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE, 0);

	 // enable interrupts for all buttons
	 IOWR_ALTERA_AVALON_PIO_IRQ_MASK(BUTTONS_BASE, 0x7);

	 // register the ISR
	 alt_irq_register(BUTTONS_IRQ,context_going_to_be_passed, handle_button_interrupts);
}


//Interupt handler for button PIO
void handle_button_interrupts(void* context, alt_u32 id){
	//cast the context pointer to an integer pointer  –
	//must be volatile to prevent unwanted compiler optimisation
	 int* temp = (int*) context;
	 // need to cast the context first before using it
	 (*temp) = IORD_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE);
	 // clear the edge capture register
	 IOWR_ALTERA_AVALON_PIO_EDGE_CAP(BUTTONS_BASE, 0);
	 printf("button: %i\n", *temp);

		 if (*temp == 1) {
			 buttonOneFlag = 1; //NS Ped button
		 } else if (*temp == 2) {
			 buttonTwoFlag = 1; //EW Ped button
		 } else if (*temp == 4 && currentMode == s_mode4) { //Vehicle button
			 if(buttonThreeFlag == 0 && out_red == 1){ //Entering intersection on R/R
				 printf("Snapshot Taken\n");
				 buttonThreeFlag = 1;
				 printST = 1;
			 } else if(buttonThreeFlag == 0){ //Entering intersection
				 printf("Vehicle Entering\n");
				 buttonThreeFlag = 1;
				 vehicleCaught = 0;

			 } else if(buttonThreeFlag == 1){ //Leaving intersection
				 buttonThreeFlag = 0;
				 printf("Vehicle Leaving\n");
				 printVL = 1;
			 }
		 }
}


//Interrupt handler for traffic light timings
alt_u32 timer_isr_function(void* context)
{

	alt_alarm_stop(&timer);
	timeout = 1;

	return 1;
}


//Millisecond timer to count vehicle duration
int ms_isr_function(void* timerContext)
{
	timeCounted++;

	return 1; // the next time out will be 1 milli-seconds
}


//Interrupt handler for 2s camera timeout
alt_u32 camera_isr_function(void* context)
{
	//Stop timer & set timeout flag
	alt_alarm_stop(&carTimer);
	timeoutCar = 0;

	//Take snapshot if vehicle has been caught
	if (buttonThreeFlag == 1) {
		vehicleCaught = 1;
		printf("Snapshot Taken\n");
		printST = 1;
	}

	return 2000; // the next time out will be 2000 milli-seconds
}


//Function to write to uart output
void writeToUART(char toPrint[], int timeCount) {
	if (uartPrint != NULL) {

		if(timeCount != NULL){
			fprintf(uartPrint, "Time: %dms \r\n \r\n", timeCount);
		} else{
			fprintf(uartPrint, "%s \r\n", toPrint);
		}



	}
}


