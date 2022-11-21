#include "core.h"
#include "uart.h"
#include "gpu.h"
#include <stdlib.h>

int main()
{
	uint8_t *framebuffer = GPUAllocateBuffer(320*240*3);
	GPUSetVPage((uint32_t)framebuffer);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

	// Output to UART port
    UARTWrite("Hello, world!\n");

	// Show on the connected display
	GPUPrintString(framebuffer, 0, 0, "Hello, world", 0x7FFFFFFF);

    return 0;
}
