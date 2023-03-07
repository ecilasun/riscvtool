// Boot ROM

#include "rvcrt0.h"

#include "memtest/memtest.h"

#include <math.h>
#include <string.h>

#include "core.h"
#include "fileops.h"
#include "uart.h"
#include "leds.h"
#include "isr.h"

int main()
{
	// Debug LEDs off
	LEDSetState(0);

	// Main ISR routine.
	// Loaded executable can always override since it runs in machine mode.
	InstallISR();

	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2J");

	{
		UARTWrite("\nClearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF

		uint64_t startclock = E32ReadTime();
		UARTWrite("Start clock:");
		UARTWriteDecimal(startclock);

		for (uint32_t m=0x0A000000; m<0x0C000000; m+=4)
			*((volatile uint32_t*)m) = 0x00000000;

		uint64_t endclock = E32ReadTime();
		UARTWrite("\nEnd clock:");
		UARTWriteDecimal(endclock);

		uint32_t deltams = ClockToMs(endclock-startclock);
		UARTWrite("\nClearing 32Mbytes took ");
		UARTWriteDecimal((unsigned int)deltams);
		UARTWrite(" ms\n");

		UARTWrite("\n-------------MemTest--------------\n");
		UARTWrite("Copyright (c) 2000 by Michael Barr\n");
		UARTWrite("----------------------------------\n");

		UARTWrite("Data bus test (0x0A000000-0x0A03FFFF)...");
		int failed = 0;
		for (uint32_t i=0x0A000000; i<0x0A03FFFF; i+=4)
		{
			failed += memTestDataBus((volatile datum*)i);
		}
		UARTWrite(failed == 0 ? "passed (" : "failed (");
		UARTWriteDecimal(failed);
		UARTWrite(" failures)\n");

		UARTWrite("Address bus test (0x0B000000-0x0B03FFFF)...");
		int errortype = 0;
		datum* res = memTestAddressBus((volatile datum*)0x0B000000, 262144, &errortype);
		UARTWrite(res == NULL ? "passed\n" : "failed\n");
		if (res != NULL)
		{
			if (errortype == 0)
				UARTWrite("Reason: Address bit stuck high at 0x");
			if (errortype == 1)
				UARTWrite("Reason: Address bit stuck low at 0x");
			if (errortype == 2)
				UARTWrite("Reason: Address bit shorted at 0x");
			UARTWriteHex((unsigned int)res);
			UARTWrite("\n");
		}

		UARTWrite("Memory device test (0x0C000000-0x0C03FFFF)...");
		datum* res2 = memTestDevice((volatile datum *)0x0C000000, 262144);
		UARTWrite(res2 == NULL ? "passed\n" : "failed\n");
		if (res2 != NULL)
		{
			UARTWrite("Reason: incorrect value read at 0x");
			UARTWriteHex((unsigned int)res2);
			UARTWrite("\n");
		}

		if ((failed != 0) | (res != NULL) | (res2 != NULL))
			UARTWrite("Memory device does not appear to be working correctly.\n");
	}

	// Load startup executable
	{
		MountDrive();
		RunExecutable(0, "sd:\\boot.elf", true);
	}

	while (1) {
		asm volatile ("wfi;");
	}

	return 0;
}
