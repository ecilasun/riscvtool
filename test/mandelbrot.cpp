#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>
#include "utils.h"
#include "gpu.h"

uint8_t mandelbuffer[256*192];

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
void mandelbrotFloat(float ox, float oy, float sx)
{
   //volatile uint32_t *VRAMDW = (uint32_t *)VRAM;
   int R = int(27.71f-5.156f*logf(sx));

   for (int row = 0; row < 192; row+=2)
   {
      for (int col = 0; col < 256; col+=2)
      {
         int M = evalMandel(R, col, row, ox, oy, sx);
         uint8_t c;
         if (M < 2)
         {
            c = MAKERGB8BITCOLOR(0, 100, 0);
         }
         else
         {
            float ratio = float(M)/float(R);
            c = MAKERGB8BITCOLOR(int(10+1.f*ratio*140), 100, int(1.f*ratio*200));
         }

         mandelbuffer[col + (row<<8)] = c;
         mandelbuffer[col+1 + (row<<8)] = c;
         mandelbuffer[col + ((row+1)<<8)] = c;
         mandelbuffer[col+1 + ((row+1)<<8)] = c;
      }
   }
}

int main(int argc, char ** argv)
{
   // Set initial page
   uint32_t page = 0;
   GPUSetRegister(6, page);
   GPUSetVideoPage(6);

   float R = 0.399999976158f;///4.0E-5f;
   float X = -0.235125f;
   float Y = 0.827215f;

   volatile unsigned int gpustate = 0x00000000;
   unsigned int cnt = 0x00000000;
   while(1)
   {
      //uint64_t clkA = ReadClock();

      // Generate one line of mandelbrot into offscreen buffer
      // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
      mandelbrotFloat(X,Y,R);
      R += 0.01f; // Zoom

      if (gpustate == cnt) // GPU work complete, push more
      {
         ++cnt;

         // DMA
         // Copies mandebrot ofscreen buffer (256x192) from SYSRAM onto VRAM, one scanline at a time
         for (uint32_t mandelscanline = 0; mandelscanline<192; ++mandelscanline)
         {
            // Source address in SYSRAM (NOTE: The address has to be in multiples of DWORD)
            uint32_t sysramsource = uint32_t(mandelbuffer+mandelscanline*256);
            GPUSetRegister(4, sysramsource);

            // Copy to top of the VRAM (Same rule here, address has to be in multiples of DWORD)
            uint32_t vramramtarget = (0 + (0+mandelscanline)*256)>>2;
            GPUSetRegister(5, vramramtarget);

            // Length of copy in DWORDs
            uint32_t dmacount = 256>>2;
            GPUKickDMA(4, 5, dmacount, 0);
         }

         // Stall GPU until vsync is reached
         GPUWaitForVsync();

         page = (page + 1)%2;
         GPUSetRegister(6, page);
         GPUSetVideoPage(6);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(&gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteSystemMemory(2, 1);

         gpustate = 0;
      }
   }

   return 0;
}
