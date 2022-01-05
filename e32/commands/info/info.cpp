#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"

#include "../cmdboot.h"

int main()
{
    UARTWrite("\n┌──────┬─────────────────────────────────────────────────────┐\n");
    UARTWrite("│ CPU  │ E32 RISC-V (rv32imfZicsr) @100Mhz                   │\n");
    UARTWrite("│ BUS  │ AXI4-Lite @100MHz                                   │\n");
    UARTWrite("├──────┼─────────────────────────────────────────────────────┤\n");
    UARTWrite("│ DDR3 │ 0x00000000-0x0FFFFFFF (RAM 256MBytes)               │\n");
    UARTWrite("│ BRAM │ 0x10000000-0x1000FFFF (RAM/ROM v0006, 64Kbytes)     │\n");
    UARTWrite("│ SRAM │ 0x10010000-0x1002FFFF (Scratchpad, 128Kbytes)       │\n");
    UARTWrite("│ UART │ 0x2000000n (n=8:R/W n=4:AVAIL n=0:FULL)             │\n");
    UARTWrite("│ SPI  │ 0x2000100n (n=0:R/W)                                │\n");
    UARTWrite("│ GPU  │ 0x40000000 (framebuffers, command streams, other)   │\n");
    UARTWrite("└──────┴─────────────────────────────────────────────────────┘\n\n");

    return 0;
}
