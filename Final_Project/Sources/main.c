/* ###################################################################
**     Filename    : main.c
**     Project     : Final_Project
**     Processor   : MK64FN1M0VLL12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2021-11-16, 13:05, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "Pins1.h"
#include "CsIO1.h"
#include "IO1.h"
#include "PWM1.h"
#include "PwmLdd1.h"
#include "TU1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
/************* Defined Constants *************/
#define MILLIVOLTS_PER_BIT 0.05035
#define MILLIAMPS_PER_MILLIVOLT 25
#define MILLIAMPS_PER_BIT 1.25881
// Duty cycle to hold the motor off
#define STARTUP_PWM_DC 19
// Lowest duty cycle that gets the motor to spin
#define MOTOR_START_DC 30
// Limit on the duty cycle that the ESC can run the motor at
#define DC_UPPER_LIMIT 80
// Limit on what the user can choose as the set point
#define MAX_SETPOINT 800
/************* Global Variables *************/
float duty_cycle = MOTOR_START_DC;
unsigned short curr_current = 0;
int curr_error = 0;
int set_point = 0;
const float Kp = 0.001;
const float Kd = 0.002;
const float Ki = 0.0001;
int last_error = 0;
int error_memory [10] = {0,0,0,0,0,0,0,0,0,0};

void software_delay(unsigned long delay)
{
    while (delay > 0) delay--;
}
void PWM_Set_Duty_Cycle(float desired_duty_cycle_percent) {
	//if (emergency_brake_active_flag)
	//	return;
	float inverted_duty_cycle = 100.0 - desired_duty_cycle_percent;
	PWM1_SetRatio16((uint16_t)(655.0 * inverted_duty_cycle));
}
void Motor_Start_Up() {
	// Send a 19% duty cycle signal on start-up
	PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
	software_delay(9000000UL);
}
unsigned short ADC_raw_val(void)
{
	ADC0_SC1A = 0x00;
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK));
	return ADC0_RA;
}
unsigned short ADC_avg_val(void) {
	char j;
	unsigned long sum = 0;
	for (j = 0; j < 32; ++j) {
		unsigned short curr = ADC_raw_val();
		sum += curr;
	}
	// Bit shifting to divide by 16
	sum = sum >> 5;
	// 49843 corresponds to approximately 0 amps
	if (sum > 49843UL) {
		return (unsigned short)(sum - 49843UL);
	}
	else {
		return 0;
	}
}
int current_in_milliamps(int current_bits) {
	return (int)(current_bits * MILLIAMPS_PER_BIT);
}
void Check_Emergency_Brake() {
	// Non-interrupt solution to emergency brake
	if ((GPIOB_PDIR & 0x00000004) == 0) {
		printf("Emergency brake is active\n");
		PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
		while(1);
	}
}
void PID_Loop() {
	curr_current = ADC_avg_val();
	// Needs to be a signed value
	curr_error = set_point - curr_current;

	// If the set point is specified as zero, then the motor is forced to stop.
	// otherwise, the motor will be kept running by maintaining a minimum duty cycle of 30%
	if (set_point == 0) {
		PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
		duty_cycle = STARTUP_PWM_DC;
		return;
	}

	float proportional = Kp * curr_error;
	float derivative = Kd * (curr_error - last_error);
	// Limit how much the derivative term can react to a rapid
	// change in the amount of error to avoid overcompensation
	if (derivative > 0.1) {
		derivative = 0.1;
	}
	if (derivative < -0.1) {
		derivative = -0.1;
	}
	last_error = curr_error;
	// Integral term is the sum of the last 10 errors
	char i;
	float integral = 0;
	for (i = 0; i < 9; ++i) {
		error_memory[i] = error_memory[i+1];
		integral += error_memory[i];
	}
	// Index 9 stores the most recent error
	error_memory[9] = curr_error;
	integral += curr_error;
	integral = Ki * integral;

	float PID_control = proportional + derivative + integral;
	// The control signal from the PID controller determines how much
	// to change the duty cycle
	duty_cycle = duty_cycle + PID_control;

	// Establish upper and lower limits
	if (duty_cycle > DC_UPPER_LIMIT) {
		duty_cycle = DC_UPPER_LIMIT;
	}
	else if (duty_cycle < MOTOR_START_DC) {
		duty_cycle = MOTOR_START_DC;
	}

	PWM_Set_Duty_Cycle(duty_cycle);
}
void Update_UI() {
	char* new_pwm_string [50];
	gcvt(duty_cycle, 6, new_pwm_string);
	printf("-----------------------------\n");
	printf("Current feedback raw: %hu \n", curr_current);
	printf("Current feedback in milliamps: %d \n", current_in_milliamps(curr_current));
	printf("Error: %d \n", curr_error);
	printf("Error in milliamps: %d \n", current_in_milliamps(curr_error));
	printf("New duty cycle: %s \n", new_pwm_string);
	printf("-----------------------------\n");
}
void Get_Digit() {
	static int input = 0;
	char digit = UART0_D;
	// Only positive integers are accepted, so ignore any input
	// that is not a valid digit or newline character
	if (digit == 13) {
		// If newline is recieved, take the integer obtained from the user
		// and make it the new setpoint
		set_point = input;
		input = 0;
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("Set point changed: %d\n", set_point);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		software_delay(4000000UL);
	}
	else if (digit >= '0' && digit <= '9') {
		// If digit is received, add it to the current user input and check that
		// the input does not exceed the maximum allowed set point
		input = (input * 10) + (digit - 48);
		if (input > MAX_SETPOINT) {
			printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			printf("User input exceeds maximum allowable set point\n");
			printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			software_delay(4000000UL);
			input = 0;
			return;
		}
	}
	else {
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("Invalid character received\n");
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		software_delay(4000000UL);
	}
}
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  // Setup required for ADC
  SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; // 0x8000000u; Enable ADC0 Clock
  ADC0_CFG1 = 0x0C; // 16bits ADC; Bus Clock
  ADC0_SC1A = 0x1F; // Disable the module, ADCH = 11111
  // Setup required for emergency break
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
  PORTB_PCR2 = 0x100;
  GPIOB_PDDR &= 0xFFFFFFFB;
  // Flush UART transmit and receive buffers
  UART0_CFIFO = 0XC0;

  printf("\n-----------------------------\n");
  printf("*****************************\n");
  printf("-----------------------------\n");

  Motor_Start_Up();

	while(1) {
		Check_Emergency_Brake();
		if (UART0_RCFIFO > 0) {
			Get_Digit();
		}
		PID_Loop();
		Update_UI();
		software_delay(1000000UL);
	}

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
