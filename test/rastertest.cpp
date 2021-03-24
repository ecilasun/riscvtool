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

void DDALine(int x0, int y0, int x1, int y1)
{
    // calculate dx , dy
    int dx = x1 - x0;
    int dy = y1 - y0;

    // Depending upon absolute value of dx & dy
    // choose number of steps to put pixel as
    // steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy)
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    // calculate increment in x & y for each steps
    int xinc = dx*4096 / steps;
    int yinc = dy*4096 / steps;

    // Put pixel for each step
    int x = x0*4096;
    int y = y0*4096;
    for (int i = 0; i <= steps; i++)
    {
        VRAM[(x>>12) + (y>>12)*256] = 0x00;
        x += xinc;
        y += yinc;
    }
}

int main(int argc, char ** argv)
{
    while(1)
    {
        ClearScreen(0xFF);
        DDALine(0,0,255,191);
        DDALine(255,0,0,191);
    }
    return 0;
}

#pragma GCC pop_options
