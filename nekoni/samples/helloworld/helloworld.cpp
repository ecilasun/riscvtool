#include "core.h"
#include "uart.h"
#include <stdio.h>

int main()
{
    EchoUART("Hello, world!\n");

    // Stay here, as we don't have anywhere else to go back to
    while (1) { }

    return 0;
}
