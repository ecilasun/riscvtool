#include "core.h"
#include "uart.h"
#include "gpu.h"
#include <math.h>
#include <stdio.h>

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
   int R = 32;//int(27.71f-5.156f*logf(sx));

   for (int row = 0; row < 128; ++row)
   {
      for (int col = 0; col < 128; col+=4)
      {
        int c[4];
        for (int i=0;i<4;++i)
        {
            int M = evalMandel(R, col+i, row, ox, oy, sx);
            if (M < 2)
            {
                c[i] = 0;
            }
            else
            {
                float ratio = float(M)/float(R);
                c[i] = 0xFF&int(1.f*ratio*255);
            }
        }

        uint32_t a = (col+row*512)>>2;
        uint32_t v = (c[3]<<24)|(c[2]<<16)|(c[1]<<8)|c[0];

        GPUSetRegister(2, v);
        GPUSetRegister(3, a);
        GPUWriteVRAM(3, 2, 0xF);
      }
   }
   return 1;
}

int main()
{
    float R = 4.0E-5f + 0.01f; // Step once to see some detail due to adaptive code
    float X = -0.235125f;
    float Y = 0.827215f;

    EchoStr("Mandelbrot\n");

    // Set grayscale palette
    for (uint32_t i=0;i<256;++i)
    {
        int j=255-i;
        uint32_t color = MAKERGBPALETTECOLOR(j, j, j);
        GPUSetRegister(0, i);
        GPUSetRegister(1, color);
        GPUSetPaletteEntry(0, 1);
    }

    // Activate page 0 for write (which will then set page 1 to display)
    int page = 0;
    GPUSetRegister(2, page%2);
    GPUSetVideoPage(2);

    while (1)
    {
        mandelbrotFloat(X,Y,R);
        // Activate page 1 to write (which will then set page 0 to display)
        page++;
        GPUSetRegister(2, page%2);
        GPUSetVideoPage(2);

        R += 0.01f; // Zoom
        EchoStr("step...\n");
    }

    // Stay here, as we don't have anywhere else to go back to
    while (1) { }

    return 0;
}
