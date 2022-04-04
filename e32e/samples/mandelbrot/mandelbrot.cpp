#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>

#include "core.h"
#include "uart.h"
#include "gpu.h"

uint8_t *mandelbuffer = nullptr;

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

         mandelbuffer[col + (row*320)] = c;
      }
   }

   row = (row+1)%240;

   return 1;

   // distance	(via iq's shadertoy sample https://www.shadertoy.com/view/lsX3W4)
	// d(c) = |Z|·log|Z|/|Z'|
	//float d = 0.5*sqrt(dot(z,z)/dot(dz,dz))*log(dot(z,z));
   //if( di>0.5 ) d=0.0;
}

int main(int argc, char ** argv)
{
	for (uint32_t i=0;i<256;++i) // Set up color palette
	{
		int j=255-i;
		GPUPAL_32[i] = MAKERGBPALETTECOLOR(j, j, j);
	}

	mandelbuffer = new uint8_t[320*240];

    uint32_t page = 0;

    float R = 4.0E-5f + 0.01f; // Step once to see some detail due to adaptive code
    float X = -0.235125f;
    float Y = 0.827215f;

    uint64_t prevreti = E32ReadRetiredInstructions();
    uint32_t prevms = ClockToMs(E32ReadTime());
    uint32_t ips = 0;
    while(1)
    {
        // Generate one line of mandelbrot into offscreen buffer
        // NOTE: It is unlikely that CPU write speeds can catch up with GPU DMA transfer speed, should not see a flicker
        mandelbrotFloat(X,Y,R);

        uint32_t ms = ClockToMs(E32ReadTime());

		if ((row%32) == 0) // Update every 32nd row
		{
			for (uint32_t i=0;i<80*240;++i)
				GPUFBWORD[i] = ((uint32_t*)mandelbuffer)[i];
			*GPUCTL = page;
	        page = (page + 1)%2;
		}

		if (row == 239)
            R += 0.001f; // Zoom

        if (ms-prevms > 1000)
        {
            prevms += 1000; // So that we have the leftover time carried over

            uint64_t reti = E32ReadRetiredInstructions();
            ips = (reti-prevreti);
            prevreti = reti;

            UARTWrite("IPS: ");
            UARTWriteDecimal(ips);
            UARTWrite("\n");
        }
    }

	delete [] mandelbuffer;

    return 0;
}
