#include <stdio.h>
#include <cstdlib>

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

	uint8_t *framebufferA = (uint8_t*)malloc(320*240*3 + 64);
	uint8_t *framebufferB = (uint8_t*)malloc(320*240*3 + 64);
	framebufferA = (uint8_t*)E32AlignUp((uint32_t)framebufferA, 64);
	framebufferB = (uint8_t*)E32AlignUp((uint32_t)framebufferB, 64);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

    int cycle = 0;
    while (1)
    {
        // Values measured are 12 bits (0..4095)
        voltagebuffer[x] = *XADCAUX0;
        temperaturebuffer[x] = *XADCTEMP;
        x++;

        // Flush data cache at last pixel so we can see a coherent image
        if (x==320)
        {
            // Video scan-out page
            uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;
            // Video write page
            uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;
            // Write page as words for faster block copy
            uint32_t *writepageword = (uint32_t*)writepage;
            // Show the read page while we're writing to the write page
            GPUSetVPage((uint32_t)readpage);

            // Clear screen to white
            for (uint32_t i=0;i<80*240;++i)
                writepageword[i] = 0x0F0F0F0F;

            // Show voltage value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = (voltagebuffer[i]/17)%240;
                writepage[i + y*320] = 0x00; // Black
            }

            // Show temperature value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = (temperaturebuffer[i]/17)%240;
                writepage[i + y*320] = 0x0A; // Green
            }

            GPUPrintString(writepage, 0, 16, "Green: temperature Black: voltage");

            x = 0;
            asm volatile( ".word 0xFC000073;");

            ++cycle;
        }
    }

    return 0;
}
