/*
 * fsm_mode2.h
 *
 *  Created on: 30/08/2019
 *      Author: sdup751
 */

#ifndef FSM_H_
#define FSM_H_

#include "sys/alt_alarm.h"

//Enums
enum state{s_1, s_2, s_3, s_4, s_5,s_6};
//1 - R,R
//2 - G,R
//3 - Y,R
//4 - R,R
//5 - R,G
//6 - R,Y
enum state currentState;
enum state nextState;

// Functions
void mode2_tick();
alt_u32 timer_isr_function(void* context);
int ms_isr_function(void* timerContext);
alt_u32 camera_isr_function(void* timerContext);

//initialise button interrupts
void init_button_interrupts();
void handle_button_interrupts(void* context, alt_u32 id);
void updateTimerValues();
void cameraFunctionality();

// Inputs
int timeout;
volatile int buttonOneFlag;
volatile int buttonTwoFlag;
volatile int buttonThreeFlag;
int sw17;

// Traffic light Outputs
int NS_G;
int NS_Y;
int NS_R;
int EW_G;
int EW_Y;
int EW_R;
int NS_ped;
int EW_ped;
int out_red;

//Traffic light times
alt_u32 NS_G_time;
alt_u32 NS_Y_time;
alt_u32 NS_R_time;
alt_u32 EW_G_time;
alt_u32 EW_Y_time;
alt_u32 EW_R_time;

//Timers
alt_alarm carTimer;
alt_alarm ms;
alt_alarm timer;
void* timerCar;
void* timerContext;
int timeoutCar;
int blockTimer;
int timeCounted;


#endif
