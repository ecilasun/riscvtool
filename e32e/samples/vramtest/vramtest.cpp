#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	uint32_t cycle = 0;

	do{
		// Pattern
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				GPUFB[x+y*320] = ((cycle + x)^y)%255;

		// Diagonal
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				if (x==y) GPUFB[x+y*320] = cycle;

		// Inverted copy of lower 32 rows to upper 32 rows of the screen
		for (int y=0;y<32;++y)
			for (int x=0;x<80;++x)
				GPUFBWORD[x+y*80] = GPUFBWORD[x+(y+208)*80] ^ 0xFFFFFFFF;

		// Overlay text
		DrawText(0, 0, "VRAM Test");

		// Flip to the other frame buffer
		FrameBufferSelect(cycle, cycle^1);
		++cycle;
	} while (1);

	return 0;
}