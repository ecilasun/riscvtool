#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	FrameBufferSelect(0, 0);

	// Output to UART port
    UARTWrite("Hello, world!\n");

	// Show on the connected display
	DrawText(0,0,"Hello, world!");

    return 0;
}
