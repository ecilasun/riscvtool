#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

void mandelbrot1(float ox, float oy, float sx)
{
   const int maxiter = 32;
   int height = 192;
   int width = 256;
   for (int row = 0; row < height; ++row) {
      for (int col = (row%2)*2; col < width; col+=4) {
         float c_re = (col - width/0.65f)*(sx+1.2f)/width + ox;
         float c_im = (row - height/2.f)*(sx+1.2f)/width + oy;
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

         unsigned char color = (unsigned char)((iteration/4)%255);
         VRAM[col+(row<<8)] = color;
         /*VRAM[col+1+(row<<8)] = color;
         VRAM[col+((row+1)<<8)] = color;*/
      }
   }
}

void mandelbrot2()
{
   for (int y = 0; y < 192; y+=2)
   {
      for (int x = 0; x < 256; x+=2)
      {
         float ca = 0.002f * float(x - 80) / 80.f - 0.7463f;
         float cb = 0.002f * float(y - 40) / 80.f + 0.1102f;
         float a = ca;
         float b = cb;
         int n = 0;
         for (; n < 64; ++n)
         {
            float ta = a * a - b * b;
            if (ta > 2.f)
               break;
            b = cb + 2 * a * b;
            a = ca + ta;
         }
         VRAM[x+(y<<8)] = (n * 3);
         VRAM[x+1+(y<<8)] = (n * 3);
         VRAM[x+((y+1)<<8)] = (n * 3);
         VRAM[x+1+((y+1)<<8)] = (n * 3);
      }
   }
}

int main(int argc, char ** argv)
{
   ClearScreen(0x00);

   float ox = 0.f;
   float oy = 0.f;
   float sx = 0.f;
   while(1)
   {
      mandelbrot1(ox,oy,sx);
      ox -= 0.05f;
      oy += 0.01f;
      sx -= 0.05f;
   }
   return 0;
}

#pragma GCC pop_options
