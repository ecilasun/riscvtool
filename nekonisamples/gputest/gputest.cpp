#include "core.h"
#include "uart.h"
#include "apu.h"
#include "gpu.h"
#include <math.h>
#include <stdio.h>

int main()
{
    EchoUART("GPU Test: Clear VRAM\n");

    // Set color 0xFF to white
    GPUSetRegister(0, 0xFF);
    GPUSetRegister(1, MAKERGBPALETTECOLOR(0xFF,0xFF,0xFF));
    GPUSetPaletteEntry(0, 1);

    for (uint32_t y=16;y<64;++y)
        for (uint32_t x=20;x<64;++x)
        {
            uint32_t tilex = x>>5;
            uint32_t tiley = y>>5;
            uint32_t sx = x&0x1F;
            uint32_t sy = y&0x1F;
            uint32_t tileoffset = (tilex+tiley*10)<<8;
            uint32_t localoffset = (sx + sy*32)/4;
            uint32_t a = tileoffset | localoffset;

            GPUSetRegister(2, 0xFFFFFFFF);
            GPUSetRegister(3, a);
            GPUWriteVRAM(3, 2, 0xF);
        }

    return 0;
}
