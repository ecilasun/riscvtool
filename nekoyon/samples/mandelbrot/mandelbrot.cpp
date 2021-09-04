#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>

#include "core.h"
#include "uart.h"
#include "gpu.h"

#define HIHALF(_x_) ((_x_&0xFFFF0000)>>16)
#define LOHALF(_x_) (_x_&0x0000FFFF)

// Place the offscreen buffer in GPU accessible memory
// This is a 128x128 back buffer to be DMAd to V-RAM
uint8_t *mandelbuffer = (uint8_t*)(GRAMStart + 0x800); // Move 8196 bytes (2048 words) away from GPU programs

GPUCommandPackage gpuSetupProg;
GPUCommandPackage dmaMandelProg;
GPUCommandPackage swapProg;

int evalMandel(const int maxiter, int col, int row, float ox, float oy, float sx)
{
   int iteration = 0;

   float c_re = (float(col) - 64.f) / 128.f * sx + ox;
   float c_im = (float(row) - 64.f) / 128.f * sx + oy;
   float x = 0.f, y = 0.f;
   float x2 = 0.f, y2 = 0.f;
   while (x2+y2 < 4.f && iteration < maxiter)
   {
      y = c_im + 2.f*x*y;
      x = c_re + x2 - y2;
      x2 = x*x;
      y2 = y*y;
      ++iteration;
   }

   return iteration;
}

// http://blog.recursiveprocess.com/2014/04/05/mandelbrot-fractal-v2/
int mandelbrotFloat(float ox, float oy, float sx)
{
   int R = int(27.71f-5.156f*logf(sx));

   for (int row = 0; row < 128; ++row)
   {
      for (int col = 0; col < 128; ++col)
      {
         int M = evalMandel(R, col, row, ox, oy, sx);
         int c;
         if (M < 2)
         {
            c = 0;
         }
         else
         {
            float ratio = float(M)/float(R);
            c = int(1.f*ratio*255);
         }

         mandelbuffer[col + (row<<7)] = c;
      }
   }

   return 1;

   // distance	(via iq's shadertoy sample https://www.shadertoy.com/view/lsX3W4)
	// d(c) = |Z|·log|Z|/|Z'|
	//float d = 0.5*sqrt(dot(z,z)/dot(dz,dz))*log(dot(z,z));
   //if( di>0.5 ) d=0.0;
}

void PrepareCommandPackages()
{
    // GPU setup program

    GPUBeginCommandPackage(&gpuSetupProg);
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE)); // Write to page 0
    for (uint32_t i=0;i<256;++i) // Set up color palette
    {
        int j=255-i;
        uint32_t color = MAKERGBPALETTECOLOR(j, j, j);
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF(color)));
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF(color)));   // setregi r15, color
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_HIGHBITS, G_R14, HIHALF(i)));
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_LOWBITS, G_R14, LOHALF(i)));       // setregi r14, i
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_WPAL, G_R15, G_R14, 0x0, 0x0));                     // wpal r9, r10
    }
    GPUEndCommandPackage(&gpuSetupProg);

    // DMA program

    int xoffset = 96;
    int yoffset = 32;
    GPUBeginCommandPackage(&dmaMandelProg);
    for (int L=0; L<128; ++L)
    {
        // Source in G-RAM (note, these are byte addresses, align appropriately as needed)
        uint32_t gramsource = uint32_t(mandelbuffer+L*128);
        GPUWriteInstruction(&dmaMandelProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF((uint32_t)gramsource)));
        GPUWriteInstruction(&dmaMandelProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF((uint32_t)gramsource)));    // setregi r15, (uint32_t)mandelbuffer
        // Target in V-RAM (note, these are byte addresses, align appropriately as needed)
        uint32_t vramramtarget = ((L+yoffset)*512)+xoffset;
        GPUWriteInstruction(&dmaMandelProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_HIGHBITS, G_R14, HIHALF((uint32_t)vramramtarget)));
        GPUWriteInstruction(&dmaMandelProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_LOWBITS, G_R14, LOHALF((uint32_t)vramramtarget)));   // setregi r14, (uint32_t)vramtarget
        // DMA - 128/4 words
        uint32_t dmacount = 32;
        GPUWriteInstruction(&dmaMandelProg, GPU_INSTRUCTION(G_DMA, G_R15, G_R14, 0x0, dmacount));                                   // dma r15, r14, dmacount
    }
    GPUEndCommandPackage(&dmaMandelProg);
}

int main(int argc, char ** argv)
{
    uint32_t page = 0;

    // One for palette setup, one for DMA of the mandelbrot buffer
    PrepareCommandPackages();

    float R = 4.0E-5f + 0.01f; // Step once to see some detail due to adaptive code
    float X = -0.235125f;
    float Y = 0.827215f;

    // Set up
    GPUClearMailbox();
    GPUSubmitCommands(&gpuSetupProg);
    GPUKick();

    //uint64_t prevreti = ReadRetiredInstructions();
    //uint32_t prevms = ClockToMs(ReadClock());
    //uint32_t ips = 0;
    while(1)
    {
        // Generate one line of mandelbrot into offscreen buffer
        // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
        if (mandelbrotFloat(X,Y,R) != 0)
            R += 0.01f; // Zoom

        //uint32_t ms = ClockToMs(ReadClock());

        /*if (ms-prevms > 1000)
        {
            prevms += 1000; // So that we have the leftover time carried over
            uint64_t reti = ReadRetiredInstructions();
            ips = (reti-prevreti);
            prevreti = reti;

            UARTWrite("IPS: ");
            UARTWriteDecimal(ips);
            UARTWrite("\n");
        }*/

        // Wait for previous work from end of last frame (or from the GPU setup program above) to be done.
        // Most likely they'll be complete by the time we get here due to the work involved in between frames.
        GPUWaitMailbox();

        // Show the mandelbrot image from G-RAM buffer
        GPUClearMailbox();
        GPUSubmitCommands(&dmaMandelProg);
        GPUKick();

        GPUWaitMailbox(); // Wait before kicking the swap program

        // Wire up a 'swap' program on the fly
        page = (page + 1)%2;
        GPUBeginCommandPackage(&swapProg);
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF(page)));
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF(page)));
        GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_MISC, G_R15, 0x0, 0x0, G_VPAGE));
        GPUEndCommandPackage(&swapProg);

        GPUClearMailbox();
        GPUSubmitCommands(&swapProg);
        GPUKick();
        // NOTE: We don't wait for GPU to be done here, which will be done on start of next iteration
    }

    return 0;
}
