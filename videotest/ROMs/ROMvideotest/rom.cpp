// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"

int main()
{
    // Clear all attributes, clear screen, print boot message
    UARTWrite("Hello, world!\n");

    // Stall
    while(1)
    { }

    return 0;
}
