#include "core.h"
#include "uart.h"
#include "apu.h"
#include "gpu.h"
#include <math.h>
#include <stdio.h>

int main()
{
    EchoUART("GPU Test\n");

    // Set color 0xFF to white
    EchoUART("Writing palette entry...");
    GPUSetRegister(0, 0xFF);
    GPUSetRegister(1, MAKERGBPALETTECOLOR(0xFF,0xFF,0xFF));
    GPUSetPaletteEntry(0, 1);
    EchoUART("done\n");

    EchoUART("Writing to VRAM...");
    for (uint32_t y=0;y<224;++y)
    {
        for (uint32_t x=0;x<320;x+=4)
        {
            uint32_t tilex = x>>5;
            uint32_t tiley = y>>5;
            uint32_t sx = x&0x1F;
            uint32_t sy = y&0x1F;
            uint32_t tileoffset = (tilex+tiley*10)<<8;
            uint32_t localoffset = (sx + sy*32)>>2;
            uint32_t a = tileoffset | localoffset;

            uint32_t t = (tilex+tiley*10)&0xFF;
            uint32_t v = (t<<24)|(t<<16)|(t<<8)|t;

            GPUSetRegister(2, v);
            GPUSetRegister(3, a);
            GPUWriteVRAM(3, 2, 0xF);
        }
    }
    EchoUART("done\n");

    EchoUART("GPU write tests complete\n");

    // Stay here, as we don't have anywhere else to go back to
    while (1) { }

    return 0;
}
