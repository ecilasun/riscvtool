#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"

#define max(A,B) (A>B?A:B)
#define min(A,B) (A>B?B:A)

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

int evalMandel(const int maxiter, int col, int row, float ox, float oy, float sx)
{
   int iteration = 0;

   float c_re = float(col - 128) / 96.f * sx + ox;
   float c_im = float(row - 96) / 96.f * sx + oy;
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

void mandelbrotFloat(float ox, float oy, float sx)
{
   for (int row = 0; row < 192; ++row)
      for (int col = (row%2)*2; col < 256; col+=4)
         VRAM[col+(row<<8)] = evalMandel(256, col, row, ox, oy, sx);
}

void mandelGrad(float ox, float oy, float sx)
{
   for (int row = 0; row < 191; ++row)
   {
      for (int col = 0; col < 255; ++col)
      {
         int i0 = evalMandel(256, col, row, ox, oy, sx);
         int i1 = evalMandel(256, col+1, row, ox, oy, sx);
         int i2 = evalMandel(256, col+1, row+1, ox, oy, sx);
         //int i3 = evalMandel(256, col, row+4, ox, oy, sx);
         VRAM[col+(row<<8)] = ((i0-i1)*(i0-i1)+(i0-i2)*(i0-i2))&0xFF;
      }
   }
}

int main(int argc, char ** argv)
{
   ClearScreen(0x00);

   // Please see http://www.cuug.ab.ca/dewara/mandelbrot/Mandelbrowser.html
   // for some more interesting XYR points
 
   float X = -0.235125;
   float Y = 0.827215;
   float R = 4.0E-5f;
 
   uint32_t cnt = 0;
   while(1)
   {
      PrintHex(0,184,cnt);
      ++cnt;
      mandelbrotFloat(X,Y,R);
      R += 0.001f; // Zoom
   }
   return 0;
}

#pragma GCC pop_options
