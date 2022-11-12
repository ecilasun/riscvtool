#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>

#include "core.h"
#include "uart.h"
#include "gpu.h"

int evalMandel(const int maxiter, int col, int row, float ox, float oy, float sx)
{
   int iteration = 0;

   float c_re = (float(col) - 160.f) / 200.f * sx + ox; // Divide by shortest side of display for correct aspect ratio
   float c_im = (float(row) - 100.f) / 200.f * sx + oy;
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
static int row = 0;
int mandelbrotFloat(float ox, float oy, float sx)
{
   int R = int(27.71f-5.156f*logf(sx));

   //for (int row = 0; row < 240; ++row)
   {
      for (int col = 0; col < 320; ++col)
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

         VRAMBYTES[col + (row*320)] = c;
      }
   }

   row = (row+1)%240;

   return 1;

   // distance	(via iq's shadertoy sample https://www.shadertoy.com/view/lsX3W4)
	// d(c) = |Z|·log|Z|/|Z'|
	//float d = 0.5*sqrt(dot(z,z)/dot(dz,dz))*log(dot(z,z));
   //if( di>0.5 ) d=0.0;
}

int main()
{
   // Grayscale palette
   for (uint32_t i=0;i<256;++i)
   {
      int j=255-i;
      GPUSetPal(i, MAKECOLORRGB24(j, j, j));
   }

   float R = 4.0E-5f + 0.01f; // Step once to see some detail due to adaptive code
   float X = -0.235125f;
   float Y = 0.827215f;

   printf("Mandelbrot test\n");

   while(1)
   {
      // Generate one line of mandelbrot into offscreen buffer
      // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
      mandelbrotFloat(X,Y,R);

      // Flush data cache after each row so we can see a coherent image
      asm volatile( ".word 0xFC000073;");

      if (row == 239)
         R += 0.001f; // Zoom
   }

   return 0;
}
