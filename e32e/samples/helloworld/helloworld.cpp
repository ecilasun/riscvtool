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
    UARTWrite("\nHello, world!\n");

	// Show on the connected display
	GPUClearScreen(framebuffer, 0x00000000);
	GPUPrintString(framebuffer, 0, 0, "Hello, world", 0x7FFFFFFF);
	// Complete framebuffer writes by invalidating & writing back D$
	asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1

    return 0;
}
