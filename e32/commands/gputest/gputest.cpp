#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"

#include "../cmdboot.h"

volatile uint32_t *GPUFB0 = (volatile uint32_t* )0x40000000;
int main()
{
    for (uint32_t y=0;y<240;++y)
    {
        uint32_t baseaddress = y*128;
        for (uint32_t x=0; x<128; ++x)
        {
            uint8_t val = (x^y);
            GPUFB0[baseaddress+x] = val | (val<<8) | (val<<16) | (val<<24);
        }
    }

    return 0;
}
