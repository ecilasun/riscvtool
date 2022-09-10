// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "uart.h"

int main()
{
	// Start with all LEDs off
	SetLEDState(0x0);

	UARTWrite("rv32i\n");

	while(1)
	{
		asm volatile("wfi;");
	}

	return 0;
}
