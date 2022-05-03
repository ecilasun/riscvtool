#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
    UARTWrite("Hello, world!\n");

	*GPUCTL = 0;
	DrawText(0,0,"Hello, world!");
	*GPUCTL = 1;

    return 0;
}
