#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	uint32_t cycle = 0;

	do {
		// Video scan-out page
		uint32_t *readpage = (uint32_t *)((uint32_t)VRAM + (cycle^1) ? 320*240 : 0);
		// Video write page
		uint32_t *writepage = (uint32_t *)((uint32_t)VRAM + (cycle^1) ? 0 : 320*240);
		// Same, as byte access
		uint8_t *writepagebyte = (uint8_t*)writepage;

		// Show next page while we're drawing into current page
		GPUSetVPage((uint32_t)readpage);

		// Pattern
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				writepagebyte[x+y*320] = ((cycle + x)^y)%255;

		// Diagonal
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				if (x==y) writepagebyte[x+y*320] = cycle;

		// Inverted copy of lower 32 rows to upper 32 rows of the screen
		for (int y=0;y<32;++y)
			for (int x=0;x<80;++x)
				writepage[x+y*80] = writepage[x+(y+208)*80] ^ 0xFFFFFFFF;

		// Flush data cache at last pixel so we can see a coherent image
		asm volatile( ".word 0xFC000073;");

		// Flip to next page
		++cycle;
	} while (1);

	return 0;
}