#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>
#include "nekoichi.h"
#include "gpu.h"

// Place the offscreen buffer in GPU accessible memory
uint8_t *mandelbuffer = (uint8_t*)GraphicsRAMStart;

int evalMandel(const int maxiter, int col, int row, float ox, float oy, float sx)
{
   int iteration = 0;

   float c_re = (float(col) - 128.f) / 256.f * sx + ox;
   float c_im = (float(row) - 96.f) / 256.f * sx + oy;
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
   int R = 32;//int(27.71f-5.156f*logf(sx));

   for (int row = 0; row < 192; ++row)
   {
      for (int col = 0; col < 256; ++col)
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

         mandelbuffer[col + (row<<8)] = c;
         mandelbuffer[col + 1 + (row<<8)] = c;
         mandelbuffer[col + ((row+1)<<8)] = c;
         mandelbuffer[col + 1 + ((row+1)<<8)] = c;
      }
   }
   return 1;
}

int main(int argc, char ** argv)
{
   InitFont();

   // Set initial page
   uint32_t page = 0;
   GPUSetRegister(6, page);
   GPUSetVideoPage(6);

   float R = 4.0E-5f + 0.01f; // Step once to see some detail due to adaptive code
   float X = -0.235125f;
   float Y = 0.827215f;

   // Make sure this lands in some unused area of the GraphicsRAM
   volatile uint32_t *gpustate = (volatile uint32_t *)GraphicsRAMEnd-8;
   *gpustate = 0x0;
   uint32_t cnt = 0x0;

   // Set grayscale palette
   for (uint32_t i=0;i<255;++i)
   {
      int j=255-i;
      uint32_t color = MAKERGBPALETTECOLOR(j, j, j);
      GPUSetRegister(1, color);
      GPUSetPaletteEntry(1, i);
   }

   uint64_t prevreti = ReadRetiredInstructions();
   uint32_t prevms = ClockToMs(ReadClock());
   uint32_t ips = 0;
   while(1)
   {
      // Generate one line of mandelbrot into offscreen buffer
      // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
      if (mandelbrotFloat(X,Y,R) != 0)
         R += 0.01f; // Zoom

      uint32_t ms = ClockToMs(ReadClock());

      if (ms-prevms > 1000)
      {
         prevms += 1000; // So that we have the leftover time carried over
         uint64_t reti = ReadRetiredInstructions();
         ips = (reti-prevreti);
         prevreti = reti;
      }

      if (*gpustate == cnt) // GPU work complete, push more
      {
         ++cnt;

         // DMA
         // Copies mandebrot ofscreen buffer (256x192) from SYSRAM onto VRAM, one scanline at a time

         // Source address in SYSRAM (NOTE: The address has to be in multiples of DWORD)
         uint32_t sysramsource = uint32_t(mandelbuffer);
         GPUSetRegister(4, sysramsource);

         // Copy to top of the VRAM (Same rule here, address has to be in multiples of DWORD)
         uint32_t vramramtarget = 0x00000000;
         GPUSetRegister(5, vramramtarget);

         // Length of copy, in DWORDs
         uint32_t dmacount = (256*192)>>2;
         GPUKickDMA(4, 5, dmacount, 0);

         PrintDMA(0, 0, "IPS: ", false);
         PrintDMADecimal(5*8, 0, ips, false);

         // Stall GPU until vsync is reached
         //GPUWaitForVsync();

         page = (page + 1)%2;
         GPUSetRegister(6, page);
         GPUSetVideoPage(6);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteToGraphicsMemory(2, 1);

         *gpustate = 0;
      }
   }

   return 0;
}
