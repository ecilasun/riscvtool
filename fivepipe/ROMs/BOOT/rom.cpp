// Bootloader

// (c)2022 Engin Cilasun

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <math.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "uart.h"
#include "xadc.h"
#include "isr.h"

// HART[1..N-1] entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}
}

// HART[0] entry point
int main()
{
	uint32_t hartid = read_csr(mhartid);

	// Only on one core
	if (hartid == 0)
	{
    // Splash screen
		/*UARTWrite("\033[H\033[0m\033[2J");
    UARTWrite("HOTAS controller v0.1 (c) Engin Cilasun\n");
		UARTWrite("CPU: 1x rv32i @100MHz\n");
		UARTWrite("BUS: AXI4 bus @100MHz\n");
		UARTWrite("RAM: 131072 bytes\n");*/
		//UARTWrite("┌─┐ ├─┤└─┘\n\n");

    // Reset debug LEDs
		SetLEDState(0x0);

		// Ready to handle hardware & software interrupts
		InstallMainISR();
	}

	while(1)
	{
    	uint32_t voltage = *XADCPORT;
    	UARTWriteDecimal(voltage);
    	UARTWrite("\n");

		// TODO: Wake up to process hardware interrupt requests
		//asm volatile("wfi;");
	}

	return 0;
}
