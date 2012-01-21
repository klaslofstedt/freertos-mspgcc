/*
  
Copyright (c) 2012, Peter A. Bigot <bigotp@acm.org>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the software nor the names of its contributors may be
  used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#include "FreeRTOS.h"
#include "task.h"
#include "clocks/ucs.h"
#include <stdio.h>
#include <string.h>

#define mainLED_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )
#define mainSHOWDCO_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

static void prvSetupHardware( void );

static void showDCO ()
{
	unsigned portBASE_TYPE ctl0a;
	unsigned portBASE_TYPE ctl0b;
	unsigned long freq_Hz;
		
	portENTER_CRITICAL();
	freq_Hz = ulBSP430ucsTrimFLL( configCPU_CLOCK_HZ, configCPU_CLOCK_HZ / 128 );
	ctl0a = UCSCTL0;
	do {
		ctl0b = UCSCTL0;
		if (ctl0a == ctl0b) {
			break;
		}
		ctl0a = UCSCTL0;
	} while (ctl0a != ctl0b);
	portEXIT_CRITICAL();

	printf("UCS: SR 0x%02x RSEL %u DCO %u MOD %u ; freq %lu\n", __read_status_register(), 0x07 & (UCSCTL1 >> 4), 0x1f & (ctl0a >> 8), 0x1f & (ctl0a >> 3), freq_Hz);
}

static portTASK_FUNCTION( vShowDCO, pvParameters )
{
	portTickType xLastWakeTime;

	( void ) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	for(;;)
	{
		vTaskDelayUntil( &xLastWakeTime, 1000 );
		showDCO();
	}
}

void main( void )
{
	unsigned portBASE_TYPE uxCounter;
	int i;
	
	prvSetupHardware();
	vParTestInitialise();

	printf("Up and running\n");
	showDCO();
	
	vStartLEDFlashTasks( mainLED_TASK_PRIORITY );

	xTaskCreate( vShowDCO, ( signed char * ) "SDCO", 300, NULL, mainSHOWDCO_TASK_PRIORITY, ( xTaskHandle * ) NULL );

	/* Start the scheduler. */
	vTaskStartScheduler();
}

void vApplicationIdleHook( void ) { }

static void prvSetupHardware( void )
{
	/* Hold off watchdog */
	WDTCTL = WDTPW + WDTHOLD;

	/* Enable XT1 functions */
	P7SEL |= BIT0;

	/* P11.0: ACLK ; P11.1: MCLK; P11.2: SMCLK ; all available on test
	 * points */
	P11SEL |= BIT0 | BIT1 | BIT2;
	P11DIR |= BIT0 | BIT1 | BIT2;

	ulBSP430ucsConfigure( configCPU_CLOCK_HZ, -1 );

	/* Hold the UART in reset during configuration */
	UCA1CTL1 |= UCSWRST;
	UCA1CTLW0 = UCSWRST | UCSSEL__ACLK;
	UCA1BRW = 3;
	UCA1MCTL = (0 * UCBRF_1) | (3 * UCBRS_1);
	P5SEL |= BIT6 | BIT7;
	UCA1CTL1 &= ~UCSWRST;
}

int
putchar (int c)
{
  /* Spin until tx buffer ready */
  while (!(UCA1IFG & UCTXIFG)) {
    ;
  }
  /* Transmit the character */
  UCA1TXBUF = c;

  return c;
}

#include "utility/led.h"

const xLEDDefn pxLEDDefn[] = {
	{ .pucPxOUT = &P1OUT, .ucBIT = BIT0 }, /* Red */
	{ .pucPxOUT = &P1OUT, .ucBIT = BIT1 }, /* Orange */
};
const unsigned char ucLEDDefnCount = sizeof(pxLEDDefn) / sizeof(*pxLEDDefn);

