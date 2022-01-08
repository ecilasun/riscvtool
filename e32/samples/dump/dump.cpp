#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "memtest/memtest.h"

int main()
{
    UARTWrite("\nDumping first 256 words @0x00000000\n");

    for (uint32_t m=0x00000000; m<0x00000100; m+=4)
    {
        UARTWriteHex(*((uint32_t*)m));
        UARTWrite(" ");
    }

    UARTWrite("\n");
    return 0;
}
