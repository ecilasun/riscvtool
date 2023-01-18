#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	const uint32_t W = 320;
	const uint32_t H = 240;
	const uint32_t mode = VIDEOMODE_320PALETTED;

	// Set up frame buffers
	// NOTE: Video scanout buffers have to be aligned at 64 byte boundary
	uint8_t *framebufferA = GPUAllocateBuffer(W*H);
	uint8_t *framebufferB = GPUAllocateBuffer(W*H);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(mode, VIDEOOUT_ENABLED)); // Mode 0, video on

	uint32_t cycle = 0;
	uint32_t prevvblankcount = GPUReadVBlankCounter();
	do {
		uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;	// Video scan-out page (GPU)
		uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;	// Video write page (CPU)

		// Wait for vsync before we flip pages
		uint32_t vblankcount = GPUReadVBlankCounter();
		if (vblankcount > prevvblankcount)
		{
			prevvblankcount = vblankcount;

			// Show the read page while we're writing to the write page
			GPUSetVPage((uint32_t)readpage);

			// Pattern
			for (uint32_t y=0;y<H;++y)
				for (uint32_t x=0;x<W;++x)
					writepage[x+y*W] = ((cycle + x)^y)%255;

			// Diagonal
			for (uint32_t y=0;y<H;++y)
				for (uint32_t x=0;x<W;++x)
					if (x==y) writepage[x+y*W] = cycle;

			// Flush data cache at last pixel to complete framebuffer writes
			// to ensure we see a complete image once this page becomes the presented one
			CFLUSH_D_L1;

			// Flip to next page
			++cycle;
		}
	} while (1);

	return 0;
}
