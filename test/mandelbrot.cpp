#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

void mandelbrotFloat(float ox, float oy, float sx)
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
            float ta = a * a - b * b;
            if (ta > 2.f)
               break;
            b = cb + 2.f * a * b;
            a = ca + ta;
         }
         VRAM[x+(y<<8)] = (n * 3);
      }
   }
}

int main(int argc, char ** argv)
{
   ClearScreen(0x00);

   float ox = -0.7463f;
   float oy = 0.1102f;
   float sx = 0.002f;
   while(1)
   {
      mandelbrotFloat(ox,oy,sx);
      //ox -= 0.01f;
      //oy += 0.01f;
      sx += 0.001f;
   }
   return 0;
}

#pragma GCC pop_options
