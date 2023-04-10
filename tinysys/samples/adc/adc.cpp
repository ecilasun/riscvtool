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

    uint32_t ch0buffer[320];
    uint32_t temperaturebuffer[320];

	uint8_t *framebufferB = GPUAllocateBuffer(320*240);
	uint8_t *framebufferA = GPUAllocateBuffer(320*240);

	struct EVideoContext vx;
	GPUSetVMode(&vx, EVM_320_Pal, EVS_Enable);

	// Set buffer B as output
	GPUSetWriteAddress(&vx, (uint32_t)framebufferA);
	GPUSetScanoutAddress(&vx, (uint32_t)framebufferB);

    int cycle = 0;
    while (1)
    {
        // Values measured are 10 bits, but we often get 0...512 range
        ch0buffer[x] = ANALOGINPUTS[0];
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
            // flip the read and write pages
            GPUSetWriteAddress(&vx, (uint32_t)writepage);
            GPUSetScanoutAddress(&vx, (uint32_t)readpage);

            // Clear screen to white
            for (uint32_t i=0;i<80*240;++i)
                writepageword[i] = 0x0F0F0F0F;

            // Show voltage value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = ch0buffer[i]/2; // 0...511 -> 0..254
                y = y>239 ? 239 : y;
                writepage[i + y*320] = 0x00; // Black
            }

            // Show temperature value
            for (uint32_t i=0;i<320;++i)
            {
                uint32_t y = (temperaturebuffer[i]/17)%240;
                writepage[i + y*320] = 0x0A; // Green
            }

            x = 0;
            CFLUSH_D_L1;

            ++cycle;
        }
    }

    return 0;
}
