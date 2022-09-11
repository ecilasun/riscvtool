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
	// TODO: Install ISR

	uint32_t hartid = read_csr(mhartid);

	// HART#0 turns off all LEDs and outputs startup message to UART
	if (hartid == 0)
	{
		SetLEDState(0x0);

		UARTWrite("\033[H\033[0m\033[2J");
		UARTWrite("┌─────────────────────────┐\n");
		UARTWrite("│          ▒▒▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
		UARTWrite("│ ████████   ▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
		UARTWrite("│ █████████  ▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
		UARTWrite("│ ████████   ▒▒▒▒▒▒▒▒▒▒▒  │\n");
		UARTWrite("│ █        ▒▒▒▒▒▒▒▒▒▒▒    │\n");
		UARTWrite("│ ██   ▒▒▒▒▒▒▒▒▒▒▒▒▒   ██ │\n");
		UARTWrite("│ ████   ▒▒▒▒▒▒▒▒▒   ████ │\n");
		UARTWrite("│ ██████   ▒▒▒▒▒   ██████ │\n");
		UARTWrite("│ ████████   ▒   ████████ │\n");
		UARTWrite("│ ██████████   ██████████ │\n");
		UARTWrite("├─────────────────────────┤\n");
		UARTWrite("│ (c)2022 Engin Cilasun   │\n");
		UARTWrite("│ HOTAS Controller v0.0   │\n");
		UARTWrite("└─────────────────────────┘\n\n");
	}

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}

	return 0;
}
