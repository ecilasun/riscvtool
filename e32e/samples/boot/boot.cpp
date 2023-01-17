#include "core.h"
#include "uart.h"
#include "ps2.h"
#include <stdlib.h>

int main()
{
	UARTWrite("Experimental boot image\nCurrent test: raw keyboard input\n");

	while(1)
	{
		if (*PS2KEYBOARDDATAAVAIL)
		{
			uint32_t code = (*PS2KEYBOARDDATA)&0xFF;
			UARTWriteHexByte(code);
			UARTWrite("\n");
		}
	};

    return 0;
}
