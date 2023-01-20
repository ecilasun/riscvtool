#include "core.h"
#include "gpu.h"
#include "dma.h"
#include "uart.h"

int main()
{
	UARTWrite("DMA test\n");

	const uint32_t W = 320;
	const uint32_t H = 240;
	const uint32_t mode = VIDEOMODE_320PALETTED;

	UARTWrite("Preparing buffers\n");

	// Create source and target buffers (using GPU functions to get aligned buffer addresses)
	uint8_t *bufferB = GPUAllocateBuffer(W*H);
	uint8_t *bufferA = GPUAllocateBuffer(W*H*2);

	// Set buffer B as output
	GPUSetVPage((uint32_t)bufferB);
	GPUSetVMode(MAKEVMODEINFO(mode, VIDEOOUT_ENABLED)); // Mode 0, video on

	// Fill buffer A with some data
	for (uint32_t y=0;y<H*2;++y)
		for (uint32_t x=0;x<W;++x)
			bufferA[x+y*W] = (x^y)%255;

	// DMA operatins work directly on memory.
	// Therefore, we need to insert a cache flush here so that
	// the writes to buffer A are all written back to RAM.
	CFLUSH_D_L1;

	// Kick a DMA copy to move buffer A contents onto buffer B
	const uint32_t blockCountInMultiplesOf16bytes = (W*H)/16;
	UARTWrite("Initiating copy loop of ");
	UARTWriteDecimal(blockCountInMultiplesOf16bytes);
	UARTWrite(" blocks\n");

	int32_t offset = 0;
	int32_t dir = 1;
	while (1)
	{
		// Copy at different lines to make the output appear to scroll
		DMACopyBlocks((uint32_t)(bufferA+offset*W), (uint32_t)bufferB, blockCountInMultiplesOf16bytes);

		// Scroll
		offset = (offset+dir)%H;
		if (offset == 0 || offset == H-1) dir = -dir;

		// This is not necessary, but used here as a way to avoid flicker
		while (DMAPending()) { asm volatile("nop;"); }
	}
	return 0;
}
