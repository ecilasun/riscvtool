#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "ps2.h"

int main()
{
    UARTWrite("\nDumping incoming keyboard scan codes\n");

    while (1)
    {
        if (*PS2KEYBOARDDATAAVAIL)
        {
            uint32_t scancode = *PS2KEYBOARDDATA;
            UARTWriteHex(scancode);
            UARTWrite(":");
        }
    }

    return 0;
}
