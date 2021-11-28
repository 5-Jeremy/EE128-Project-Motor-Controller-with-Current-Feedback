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
// milliamps per bit = 1.25881
// With no current, the ADC reads around 49843 = 2.51 volts
#define MILLIVOLTS_PER_BIT 0.05035
#define MILLIAMPS_PER_MILLIVOLT 25
#define MILLIAMPS_PER_BIT 1.25881
#define STARTUP_PWM_DC 19
// Lowest duty cycle that gets the motor to spin
#define MOTOR_START_DC 30
// Limit on the duty cycle that the ESC can run the motor at
#define DC_UPPER_LIMIT 80
/************* Global Variables *************/
const float Kp = 0.001;
const float Kd = 0.002;
const float Ki = 0.0001;
float last_error = 0;
float error_memory [10] = {0,0,0,0,0,0,0,0,0,0};
// Global flag for emergency brake
unsigned char emergency_brake_active_flag;

void software_delay(unsigned long delay)
{
    while (delay > 0) delay--;
}
void PWM_Set_Duty_Cycle(float desired_duty_cycle_percent) {
	if (emergency_brake_active_flag)
		return;
	float inverted_duty_cycle = 100.0 - desired_duty_cycle_percent;
	PWM1_SetRatio16((uint16_t)(655.0 * inverted_duty_cycle));
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
void Check_Emergency_Break() {
	// Non-interrupt solution to emergency brake
	if ((GPIOB_PDIR & 0x00000004) == 0) {
		printf("Emergency brake is active\n");
		PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
		while(1);
	}
}
void PID_Loop() {
	unsigned short curr_current = ADC_avg_val();
	// Needs to be a signed value
	float curr_error = set_point - curr_current;
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
	char i;
	float integral = 0;
	for (i = 0; i < 9; ++i) {
		error_memory[i] = error_memory[i+1];
		integral += error_memory[i];
	}
	// index 9 stores the most recent error
	error_memory[9] = curr_error;
	integral += curr_error;
	integral = Ki * integral;
	// The control signal from the PID controller
	float PID_control = proportional + derivative + integral;

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
	char* new_err_string [50];
	gcvt(curr_error, 6, new_err_string);
	char* new_pwm_string [50];
	gcvt(duty_cycle, 6, new_pwm_string);
	printf("-----------------------------\n");
	printf("Current feedback: %hu \n", curr_current);
	printf("Error: %s \n", new_err_string);
	printf("New duty cycle: %s \n", new_pwm_string);
	printf("-----------------------------\n");
}
// Interrupt Service Routine for emergency break
/*
void PORTB_IRQHandler(void) {
	//Duty cycle down to 19.
	const float throttle_off = 19;
	PWM_Set_Duty_Cycle(throttle_off);
	emergency_brake_active_flag = 1;
	PORTB_ISFR = (1 << 1);
}
*/
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
  PORTB_PCR2 = 0x100; // Interrupt on rising edge on port B pin 2
  GPIOB_PDDR &= 0xFFFFFFFB;
  //PORTB_ISFR = (1 << 1);
  //emergency_brake_active_flag = 0;

  printf("\n-----------------------------\n");
  printf("*****************************\n");
  printf("-----------------------------\n");

	// Send a 19% duty cycle signal on start-up
	PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
	software_delay(5000000UL);

	// Start-up motor
	PWM_Set_Duty_Cycle(MOTOR_START_DC);
	software_delay(5000000UL);

	// set_point cannot go above 800
	int set_point = 400;
	float duty_cycle;

	while(1) {
		Check_Emergency_Break();
		PID_Loop();
		Update_UI();
		software_delay(1000000UL);
	}

/*
	char j;
	unsigned long sum;
	for (i = 30; i < 30 + 20; ++i) {
		PWM_Set_Duty_Cycle(i);

		sum = 0;
		for (j = 0; j < 16; ++j) {
			sum += ADC_raw_val();
		}
		printf("%hu \n", sum >> 4 );

		software_delay(9000000UL);
	}

	software_delay(1000000UL);

	// Throttle off
	PWM_Set_Duty_Cycle(STARTUP_PWM_DC);
*/
	while(1);


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
