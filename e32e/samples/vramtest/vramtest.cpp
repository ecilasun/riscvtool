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
				GPUFB[x+y*320] = ((cycle + x)^y)%255;

		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				if (x==y) GPUFB[x+y*320] = cycle;

		for (int x=0;x<80;++x)
			GPUFBWORD[x] = 0xFFFFFFFF;
		for (int x=0;x<80;++x)
			GPUFBWORD[x+128] = 0x20FF20FF;

		DrawText(0, 0, "VRAM Test");

		FrameBufferSelect(cycle, cycle^1);
		++cycle;
	} while (1);

	return 0;
}