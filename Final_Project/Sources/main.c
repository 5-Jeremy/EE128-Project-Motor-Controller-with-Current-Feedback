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

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
	// Send a 19% duty cycle signal on start-up to set it as minimum throttle
	const char startup_pwm_DC = 19;
	PWM_Set_Duty_Cycle(startup_pwm_DC);
	software_delay(1000000UL);

	// Lowest duty cycle that gets the motor to spin
		// corresponds to 0.18 amps
	const char motor_start_DC = 30;
	// Highest duty cycle at which the motor increases in speed
	// corresponds to 0.75 amps
	const char motor_max_DC = 82;




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
