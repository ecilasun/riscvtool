#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>
#include "utils.h"
#include "gpu.h"
#include "rom_nekoichi_rvcrt0.h"

//#pragma GCC push_options
//#pragma GCC optimize ("align-functions=16")

uint8_t mandelbuffer[256*192];

int evalMandel(const int maxiter, int col, int row, float ox, float oy, float sx)
{
   int iteration = 0;

   float c_re = float(col - 128) / 256.f * sx + ox;
   float c_im = float(row - 96) / 192.f * sx + oy;
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

   //for (int row = 64; row < 128; ++row)
   static int row = 0;

      for (int col = 0; col < 256; ++col)
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
      }
   
   ++row;
   row %= 192;
}

int main(int argc, char ** argv)
{
   float X = -0.235125;
   float Y = 0.827215;
   float R = 4.0E-5f;

   // Set register g1 with color data
   uint8_t bgcolor = 0x00;
   uint32_t colorbits = (bgcolor<<24) | (bgcolor<<16) | (bgcolor<<8) | bgcolor;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(colorbits));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(colorbits));

   // CLS
   GPUFIFO[5] = GPUOPCODE(GPUCLEAR, 1, 0, 0);  // clearvram g1

   while(1)
   {
      // DMA
      // Copies mandebrot ofscreen buffer (256x192) from SYSRAM onto VRAM, one scanline at a time
      for (uint32_t mandelscanline = 0; mandelscanline<192; ++mandelscanline)
      {
         // Source address in SYSRAM (NOTE: The address has to be in multiples of DWORD)
         uint32_t sysramsource = uint32_t(mandelbuffer+mandelscanline*256)>>2;
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 4, GPU22BITIMM(sysramsource));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 4, 4, GPU10BITIMM(sysramsource));

         // Copy to top of the VRAM (Same rule here, address has to be in multiples of DWORD)
         uint32_t vramramtarget = (0 + (0+mandelscanline)*256)>>2;
         GPUFIFO[2] = GPUOPCODE(GPUSETREGISTER, 0, 5, GPU22BITIMM(vramramtarget));
         GPUFIFO[3] = GPUOPCODE(GPUSETREGISTER, 5, 5, GPU10BITIMM(vramramtarget));

         // Length of copy in DWORDs
         uint32_t dmacount = 256>>2;
         GPUFIFO[4] = GPUOPCODE(GPUSYSDMA, 4, 5, (dmacount&0x3FFF)); // sysdma g2, g3, dmacount
      }

      // Generate one line of mandelbrot into offscreen buffer
      // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
      mandelbrotFloat(X,Y,R);
      R += 0.001f; // Zoom

      // Stall GPU until vsync is reached (should probably be before the mandelbrot)
      //GPUFIFO[4] = GPUOPCODE(GPUVSYNC, 0, 0, 0);
   }

   return 0;
}

//#pragma GCC pop_options
