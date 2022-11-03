#include <stdio.h>

#include "core.h"
#include "uart.h"
#include "gpu.h"
#include "xadc.h"

int main( int argc, char **argv )
{
	uint32_t prevVoltage = 0xC004CAFE;
	uint32_t voltage = 0x00000000;
	uint32_t x = 0;

    printf("AUX0 voltage display\n");

    while (1)
    {
        voltage = (voltage + *XADCPORT)>>1;
        if (prevVoltage != voltage)
        {
            // Clear the line ahead
            for (int k=0;k<240;++k)
                VRAMBYTES[(x+1)%320 + k*320] = 0xFF;

            // Show voltage value
            uint32_t y = (voltage/273)%240;
            VRAMBYTES[x + y*320] = 0x0F;

            // Flush data cache at last pixel so we can see a coherent image
            if (x==320)
                asm volatile( ".word 0xFC000073;");

            // Next column
            x = (x+1)%320;
            prevVoltage = voltage;
        }
    }

    return 0;
}
