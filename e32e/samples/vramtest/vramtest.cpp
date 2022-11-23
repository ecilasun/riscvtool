#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	// Set up frame buffers
	// NOTE: Video scanout buffers have to be aligned at 64 byte boundary
	uint8_t *framebufferA = (uint8_t*)malloc(320*240*3 + 64);
	uint8_t *framebufferB = (uint8_t*)malloc(320*240*3 + 64);
	framebufferA = (uint8_t*)E32AlignUp((uint32_t)framebufferA, 64);
	framebufferB = (uint8_t*)E32AlignUp((uint32_t)framebufferB, 64);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

	uint32_t cycle = 0;
	uint32_t prevvblankcount = GPUReadVBlankCounter();
	do {
		// Video scan-out page
		uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;

		// Video write page
		uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;

		// Write page as words for faster block copy
		uint32_t *writepageword = (uint32_t*)writepage;

		// Wait for vsync before we flip pages
		while (prevvblankcount == GPUReadVBlankCounter()) { asm volatile ("nop;"); }
		prevvblankcount = GPUReadVBlankCounter();

		// Show the read page while we're writing to the write page
		GPUSetVPage((uint32_t)readpage);

		// Pattern
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				writepage[x+y*320] = ((cycle + x)^y)%255;

		// Diagonal
		for (int y=0;y<240;++y)
			for (int x=0;x<320;++x)
				if (x==y) writepage[x+y*320] = cycle;

		// Inverted copy of lower 32 rows to upper 32 rows of the screen
		// This is used to test flicker when single buffered and
		// should not flicker when double buffered (page swap)
		for (int y=0;y<32;++y)
			for (int x=0;x<80;++x)
				writepageword[x+y*80] = writepageword[x+(y+208)*80] ^ 0xFFFFFFFF;

    	GPUPrintString(writepage, 8, 8, "vram test", 0x7FFFFFFF);

		// Flush data cache at last pixel so we can see a coherent image
		asm volatile( ".word 0xFC000073;");

		// Flip to next page
		++cycle;
	} while (1);

	return 0;
}