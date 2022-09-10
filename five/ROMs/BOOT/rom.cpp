// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
//#include "uart.h"

// Per-HART entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while(1)
	{
		// TODO: Per-HART stuff
	}
}

// Common entry point
int main()
{
	uint32_t hartid = read_csr(mhartid);

	// HART#0 turns off all LEDs
	if (hartid == 0) SetLEDState(0x0);

	while(1)
	{
		asm volatile("wfi;");
	}

	return 0;
}
