// Bootloader

// (c)2022 Engin Cilasun

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "uart.h"
#include "isr.h"

// Per-HART entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}
}

// Common entry point
int main()
{
	uint32_t hartid = read_csr(mhartid);

	// Only on one core
	if (hartid == 0)
	{
		SetLEDState(0x0);

		UARTWrite("\033[H\033[0m\033[2J");
		UARTWrite("CPU: 2x rv32i @50MHz\n");
		UARTWrite("BUS: AXI4 bus @50MHz\n");
		UARTWrite("RAM: 65536 bytes\n");
		//UARTWrite("┌─┐ ├─┤└─┘\n\n");

		// Ready to handle hardware & software interrupts
		InstallMainISR();
	}

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}

	return 0;
}
