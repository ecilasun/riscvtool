#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "ps2.h"
#include "ringbuffer.h"

// Keyboard event ring buffer (1024 bytes)
uint8_t *keyboardringbuffer = (uint8_t*)0x80010000;

int main()
{
    UARTWrite("\nDumping incoming keyboard events\n");

    while (1)
    {
	    uint32_t val;
        swap_csr(mie, MIP_MSIP | MIP_MTIP);
        uint32_t R = RingBufferRead(keyboardringbuffer, &val, 4);
        swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);
    	if (R)
        {
            UARTWriteHex(val);
            UARTWrite(":");
        }
    }

    return 0;
}
