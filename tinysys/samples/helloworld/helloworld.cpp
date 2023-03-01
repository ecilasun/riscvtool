#include "core.h"
#include "uart.h"

int main()
{
	// Output to UART port
    UARTWrite("Hello, world!\n");

    return 0;
}
