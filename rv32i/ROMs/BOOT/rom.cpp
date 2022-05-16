// Boot ROM

#include "rvcrt0.h"
#include "uart.h"

int main()
{
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
	UARTWrite("├─────────────────────────╡\n");
	UARTWrite("│ Minimal RISC-V (rv32i)  │\n");
	UARTWrite("│ (c)2022 Engin Cilasun   │\n");
	UARTWrite("└─────────────────────────┘\n\n");

	// Loop forever
	while(1) { }

	return 0;
}
