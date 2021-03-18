#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"

void mandelbrot1()
{
   const int maxiter = 96;
   int height = 192;
   int width = 256;
   for (int row = 0; row < height; row++) {
      for (int col = row%2; col < width; col+=2) {
         float c_re = (col - width/0.65f)*1.2f/width;
         float c_im = (row - height/2.f)*1.2f/width;
         float x = 0.f, y = 0.f;
         int iteration = 0;
         while (x*x+y*y <= 4.f && iteration < maxiter)
         {
               float x_new = x*x - y*y + c_re;
               y = 2.f*x*y + c_im;
               x = x_new;
               iteration++;
         }

         unsigned char color = (unsigned char)((iteration/4)%255);
         VRAM[col+(row<<8)] = color;
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
   ClearScreen(0xC8);

   mandelbrot1();

   while(1)
   {
   }
   return 0;
}
