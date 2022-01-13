#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
  uint32_t cycle = 0;

  do{
    for (int y=0;y<240;++y)
      for (int x=0;x<320;++x)
        GPUFB0[x+y*512] = ((cycle + x)^y)%255;

    for (int y=0;y<240;++y)
      for (int x=0;x<320;++x)
        if (x==y) GPUFB0[x+y*512] = cycle;
    ++cycle;
  } while (1);

  return 0;
}