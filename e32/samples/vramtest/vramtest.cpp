#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
  uint32_t cycle = 0;

  volatile uint32_t *GPUFB0_32 = (volatile uint32_t *)GPUFB0;

  do{
    for (int y=0;y<240;++y)
      for (int x=0;x<320;++x)
        GPUFB0[x+y*512] = ((cycle + x)^y)%255;

    for (int y=0;y<240;++y)
      for (int x=0;x<320;++x)
        if (x==y) GPUFB0[x+y*512] = cycle;

    for (int x=0;x<128;++x)
        GPUFB0_32[x] = 0xFFFFFFFF;
    for (int x=0;x<128;++x)
        GPUFB0_32[x+128] = 0x20FF20FF;
    ++cycle;
  } while (1);

  return 0;
}