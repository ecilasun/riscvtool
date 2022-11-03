#include <stdio.h>
#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main( int argc, char **argv )
{
	uint32_t cycle = 0;

	printf("Video scanout test\n");

	do {
		// Video scan-out page
		volatile uint32_t *readpage = (volatile uint32_t *)((uint32_t)VRAM + (cycle^1) ? 80*240 : 0);
		// Video write page
		volatile uint32_t *writepage = (volatile uint32_t *)((uint32_t)VRAM + (cycle^1) ? 0 : 80*240);
		// Same, as byte access
		volatile uint8_t *writepagebyte = (volatile uint8_t*)writepage;

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

		// Flip
		++cycle;
	} while (1);

	return 0;
}