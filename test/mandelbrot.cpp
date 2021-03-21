#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

void fractalFloat(float ox, float oy, float sx)
{
   for (int y = 0; y < 192; ++y)
   {
      for (int x = 0; x < 256; ++x)
      {
         float ca = sx * float(x - 128) / 128.f + ox;
         float cb = sx * float(y - 96) / 128.f + oy;
         float a = ca;
         float b = cb;
         int n = 0;
         for (; n < 64; ++n)
         {
            float ta = a*a - b*b;
            if (ta > 2.f)
               break;
            b = cb + 2.f * a * b;
            a = ca + ta;
         }
         VRAM[x+(y<<8)] = (n * 3);
      }
   }
}

void mandelbrotFloat(float ox, float oy, float sx)
{
   const int maxiter = 256;
   int height = 192;
   int width = 256;
   for (int row = 0; row < height; ++row) {
      for (int col = 0; col < width; ++col) {
      //for (int col = (row%2)*2; col < width; col+=4) {
         float c_re = float(col - 128) / 96.f * sx + ox;
         float c_im = float(row - 96) / 96.f * sx + oy;
         int iteration = 0;

         float x = 0.f, y = 0.f;
         float x2 = 0.f, y2 = 0.f;
         while (x2+y2 <= 4.f && iteration < maxiter)
         {
            y = 2.f*x*y + c_im;
            x = x2 - y2 + c_re;
            x2 = x*x;
            y2 = y*y;
            iteration++;
         }

         VRAM[col+((191-row)<<8)] = iteration;
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
 
   while(1)
   {
      mandelbrotFloat(X,Y,R);
      R += 0.001f; // Zoom
   }
   return 0;
}

#pragma GCC pop_options
