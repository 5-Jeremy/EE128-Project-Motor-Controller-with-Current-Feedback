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
void software_delay(unsigned long delay)
{
    while (delay > 0) delay--;
}
void PWM_Set_Duty_Cycle(unsigned char desired_duty_cycle_percent) {
	unsigned char inverted_duty_cycle = 100 - desired_duty_cycle_percent;
	//PWM1_SetRatio16((65535UL/100) * inverted_duty_cycle);
	PWM1_SetRatio16(655 * inverted_duty_cycle);
}
unsigned short ADC_raw_val(void)
{
	//ADC0_SC1A = 0x1A; //Write to SC1A to start conversion from ADC_0
	ADC0_SC1A = 0x00;
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK));
	return ADC0_RA;
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
  
  ////////////////
  // Ports in use are B
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
	// Configure the GPIO pins
	PORTB_PCR1 = 0xC0100;	  // Configure PORTA[1] for GPIO & Interrupt on 
                                   //when logic 1

	GPIOB_PDDR &= 0xFFFFFFFB;      // Set it as inputs

		// Clear ISFR and enable interrupts
	PORTB_ISFR = (1 << 1);
	NVIC_EnableIRQ(PORTA_IRQn);
  ////////////////
  
  
  // Setup
  SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; // 0x8000000u; Enable ADC0 Clock
  ADC0_CFG1 = 0x0C; // 16bits ADC; Bus Clock
  ADC0_SC1A = 0x1F; // Disable the module, ADCH = 11111

	// Send a 19% duty cycle signal on start-up to set it as minimum throttle
	const char startup_pwm_DC = 19;
	PWM_Set_Duty_Cycle(startup_pwm_DC);
	software_delay(100000UL);

	// Lowest duty cycle that gets the motor to spin
		// corresponds to 0.18 amps
	const char motor_start_DC = 30;
	// Highest duty cycle at which the motor increases in speed
	// corresponds to 0.75 amps
	const char motor_max_DC = 82;

	char i, j;
	unsigned long sum;
	for (i = 0; i < 64; ++i) {
		PWM_Set_Duty_Cycle(startup_pwm_DC  + i);
		sum = 0;
		for (j = 0; j < 16; ++j) {
			sum += ADC_raw_val();
		}
		printf("%hu \n", sum >> 4 );
		software_delay(500000UL);
	}


	/*
	// Code for testing motor range
	char i;
	for (i = 0; i < 64; ++i) {
		PWM_Set_Duty_Cycle(19 + i);
		printf("%d",19 + i);
		printf("\n");
		software_delay(500000UL);
	}
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
