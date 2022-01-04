#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"

#include "../cmdboot.h"

volatile uint32_t *GPUFB0_32 = (volatile uint32_t* )0x40000000;
volatile uint8_t *GPUFB0_8 = (volatile uint8_t* )0x40000000;
int main()
{
    // Full VRAM write (including outer 48 word region of each scanline)
    for (uint32_t y=0;y<240;++y)
    {
        uint32_t baseaddress = y*128;
        for (uint32_t x=0; x<128; ++x)
        {
            uint8_t val = (x^y);
            GPUFB0_32[baseaddress+x] = val | (val<<8) | (val<<16) | (val<<24);
        }
    }

    // Diagonal line to test byte ordering (only in visible VRAM region)
    for (uint32_t y=0;y<240;++y)
    {
        uint32_t baseaddress = y*512;
        for (uint32_t x=0; x<320; ++x)
        {
            GPUFB0_8[baseaddress+x] = 0xFF;
        }
    }

    return 0;
}
