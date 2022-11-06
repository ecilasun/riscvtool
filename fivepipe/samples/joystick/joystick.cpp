#include <stdio.h>

#include "core.h"
#include "uart.h"
#include "gpu.h"
#include "xadc.h"

int main( int argc, char **argv )
{
	uint32_t x = 0;

    printf("AUX0 voltage: black\n");
    printf("Temperature: green\n");

    uint32_t voltagebuffer[320];
    uint32_t temperaturebuffer[320];

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
            for (uint32_t i=0;i<80*240;++x)
                VRAM[i] = 0x0F;

            // Show voltage value
            for (uint32_t i=0;i<320;++x)
            {
                uint32_t y = (voltagebuffer[i]/17)%240;
                VRAMBYTES[i + y*320] = 0x00; // Black
            }

            // Show temperature value
            for (uint32_t i=0;i<320;++x)
            {
                uint32_t y = (temperaturebuffer[i]/17)%240;
                VRAMBYTES[i + y*320] = 0x0A; // Green
            }

            asm volatile( ".word 0xFC000073;");

            x = 0;
        }
    }

    return 0;
}
