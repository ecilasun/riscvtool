#include "core.h"
#include "uart.h"

uint32_t mytimerthing(const uint32_t hartID)
{
	UARTWrite("Tick: HART#");
	UARTWriteDecimal(hartID);
	UARTWrite("\n");

	return 1; // NOTE: Return zero to be ternimated after execution
}

int main()
{
	// Install the TISR on HART#1 & HART#2

	// HART#1 goes at one second intervals
	InstallTimerISR(1, mytimerthing, ONE_SECOND_IN_TICKS);

	// HART#2 goes at half second intervals
	InstallTimerISR(2, mytimerthing, ONE_SECOND_IN_TICKS/2);

	// Wait until HART#1&HART#2 clear in response
	UARTWrite("Waiting for HART#1\n");
	while(HARTMAILBOX[1] != 0x0)
		asm volatile("nop;");
	while(HARTMAILBOX[2] != 0x0)
		asm volatile("nop;");

	UARTWrite("HART#1 acknowledged TISR installation, HART#0 sleeping.\n");
	while(1)
	{
		// Nothing to do here
		asm volatile("wfi;");
	}

	return 0;
}
