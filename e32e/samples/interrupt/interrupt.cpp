#include "core.h"
#include "uart.h"

uint32_t mytimerthing(const uint32_t hartID)
{
	UARTWrite("Tick: HART#");
	UARTWriteDecimal(hartID);
	UARTWrite("\n");

	return 1; // NOTE: Return zero to be terminated after execution
}

int main()
{
	// Install the TISR on all HARTs
	InstallTimerISR(1, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*3);
	InstallTimerISR(2, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*4);
	InstallTimerISR(3, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*5);
	InstallTimerISR(4, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*6);
	InstallTimerISR(5, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*7);
	InstallTimerISR(6, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*8);
	InstallTimerISR(7, mytimerthing, HUNDRED_MILLISECONDS_IN_TICKS*9);

	// Wait until all HARTs clear in response
	UARTWrite("Waiting for all HARTs\n");
	while(HARTMAILBOX[1] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[2] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[3] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[4] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[5] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[6] != 0x0) asm volatile("nop;");
	while(HARTMAILBOX[7] != 0x0) asm volatile("nop;");

	UARTWrite("All HARTs acknowledged TISR installation, HART#0 going to sleep.\n");
	while(1)
	{
		// Nothing to do here
		asm volatile("wfi;");
	}

	return 0;
}
