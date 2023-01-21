#include "core.h"
#include "uart.h"
#include "ps2.h"
#include <stdlib.h>

int main()
{
	UARTWrite("┌───────────────────┐\n");
	UARTWrite("│ Auto boot sample  │\n");
	UARTWrite("│ PS2 Keyboard test │\n");
	UARTWrite("└───────────────────┘\n\n");

	while(1)
	{
		if (*PS2KEYBOARDDATAAVAIL)
		{
			uint32_t code = (*PS2KEYBOARDDATA)&0xFF;
			UARTPutChar(PS2ScanToASCII(code, 0));
		}
	};

    return 0;
}
