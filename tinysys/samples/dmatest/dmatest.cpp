#include "basesystem.h"
#include "core.h"
#include "gpu.h"
#include "dma.h"
#include "uart.h"

int main(int argc, char *argv[])
{
	UARTWrite("DMA test\n");

	const uint32_t W = 320;
	const uint32_t H = 240;

	UARTWrite("Preparing buffers\n");

	// Create source and target buffers (using GPU functions to get aligned buffer addresses)
	uint8_t *bufferB = GPUAllocateBuffer(W*H);
	uint8_t *bufferA = GPUAllocateBuffer(W*H*3);

	struct EVideoContext vx;
	GPUSetVMode(&vx, EVM_320_Pal, EVS_Enable);

	// Set buffer B as output
	GPUSetWriteAddress(&vx, (uint32_t)bufferA);
	GPUSetScanoutAddress(&vx, (uint32_t)bufferB);
	GPUSetDefaultPalette(&vx);

	// Fill buffer A with some data
	for (uint32_t y=0;y<H*3;++y)
		for (uint32_t x=0;x<W;++x)
			bufferA[x+y*W] = (x^y)%255;

	// DMA operatins work directly on memory.
	// Therefore, we need to insert a cache flush here so that
	// the writes to buffer A are all written back to RAM.
	CFLUSH_D_L1;

	// Kick a DMA copy to move buffer A contents onto buffer B
	const uint32_t blockCountInMultiplesOf16bytes = (W*H)/16;
	UARTWrite("Initiating copy loop of ");
	// Figure out how many DMAs this splits into
	const uint32_t leftoverDMA = blockCountInMultiplesOf16bytes%256;
	const uint32_t fullDMAs = blockCountInMultiplesOf16bytes/256;
	UARTWriteDecimal(fullDMAs);
	UARTWrite("*256*16byte blocks and ");
	UARTWriteDecimal(leftoverDMA);
	UARTWrite("*1*16byte block for a total of ");
	UARTWriteDecimal(fullDMAs*4096+leftoverDMA*16);
	UARTWrite("bytes\n");

	int32_t offset = 0;
	int32_t dir = 2;
	int32_t averagetime = 131072;
	int32_t outstats = 0;
	uint64_t starttime = E32ReadTime();
	uint32_t prevvsync = GPUReadVBlankCounter();
	while (1)
	{
		// Wait until there are no more DMA operations in flight
		while (DMAPending()) { asm volatile("nop;"); }

		uint64_t endtime = E32ReadTime();
		averagetime = (averagetime + (uint32_t)(endtime-starttime))/2;

		if (outstats % 512 == 0)
		{
			UARTWrite("DMA clocks (average): ");
			UARTWriteDecimal(averagetime);
			UARTWrite("\n");
		}
		++outstats;

		starttime = E32ReadTime();

		// Do the full DMAs first
		uint32_t fulloffset = 0;
		for (uint32_t full=0;full<fullDMAs;++full)
		{
			DMACopy4K((uint32_t)(bufferA+offset*W+fulloffset), (uint32_t)bufferB+fulloffset);
			fulloffset += 256*16; // 16 bytes for each 256-block, total of 4K
		}
		
		// Partial DMA next
		if (leftoverDMA!=0)
			DMACopy((uint32_t)(bufferA+offset*W+fulloffset), (uint32_t)(bufferB+fulloffset), leftoverDMA);

		// Tag for DMA sync (essentially an item in FIFO after last DMA so we can check if DMA is complete when this drains)
		DMATag(0x0);

		// Wait for vsync on the CPU side
		// Ideally one would install a vsync handler and swap pages based on that instead of stalling like this
		if (argc<=1)
		{
			uint32_t currentvsync;
			do {
				currentvsync = GPUReadVBlankCounter();
			} while (currentvsync == prevvsync);
			prevvsync = currentvsync;
		}

		// Handle Scroll
		offset = (offset+dir);
		if (offset == 0 || offset == H*2-2) dir = -dir;
	}
	return 0;
}
