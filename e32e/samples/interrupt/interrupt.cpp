#include "core.h"
#include "uart.h"

void mytimerthing()
{
	UARTWrite("HART#1 Tick\n");
}

int main()
{
	uint32_t hartid = 1;
	uint32_t TISR = (uint32_t)mytimerthing;
	uint32_t TISR_Interval = ONE_SECOND_IN_TICKS;

	// Install a timer interrupt routione for HART#1
	HARTMAILBOX[hartid*HARTPARAMCOUNT+0+NUM_HARTS] = TISR;
	HARTMAILBOX[hartid*HARTPARAMCOUNT+1+NUM_HARTS] = TISR_Interval;
	HARTMAILBOX[hartid] = 0x00000001;
	HARTIRQ[hartid] = 1; // IRQ to wake up HART#1's ISR

	// Wait until HART#1 clears in response
	UARTWrite("Waiting for HART#1\n");
	while(HARTMAILBOX[hartid] == 0x00000001)
	{
		asm volatile("nop;");
	}

	UARTWrite("HART#1 acknowledged TISR installation, HART#0 sleeping.\n");
	while(1)
	{
		// Nothing to do here
		asm volatile("wfi;");
	}

	return 0;
}
