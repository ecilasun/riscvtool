#include <stdio.h>
#include <cstdlib>

#include "core.h"
#include "uart.h"
#include "gpu.h"
#include "xadc.h"

#define EAlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))
uint8_t *framebuffer;

int main( int argc, char **argv )
{
	uint32_t x = 0;

    printf("AUX0 voltage: black\n");
    printf("Temperature: green\n");

    uint32_t voltagebuffer[320];
    uint32_t temperaturebuffer[320];

    framebuffer = (uint8_t*)malloc(320*240*3 + 64);
    framebuffer = (uint8_t*)EAlignUp((uint32_t)framebuffer, 64);
    uint32_t *framebufferword = (uint32_t*)framebuffer;
    GPUSetVPage((uint32_t)framebuffer);
    GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

    while (1)
    {
        // Values measured are 12 bits (0..4095)
        voltagebuffer[x] = *XADCAUX0;
        temperaturebuffer[x] = *XADCTEMP;
        x++;

        // Flush data cache at last pixel so we can see a coherent image
        if (x==320)
        {
            // Clear screen to white
            for (uint32_t i=0;i<80*240;++i)
                framebufferword[i] = 0x0F0F0F0F;

            // Show voltage value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = (voltagebuffer[i]/17)%240;
                framebuffer[i + y*320] = 0x00; // Black
            }

            // Show temperature value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = (temperaturebuffer[i]/17)%240;
                framebuffer[i + y*320] = 0x0A; // Green
            }

            x = 0;
            asm volatile( ".word 0xFC000073;");
        }
    }

    return 0;
}
