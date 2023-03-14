#include "core.h"
#include "gpu.h"
#include "dma.h"
#include "uart.h"

int main()
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
	UARTWriteDecimal(blockCountInMultiplesOf16bytes);
	UARTWrite(" blocks\n");

	int32_t offset = 0;
	int32_t dir = 2;
	int32_t averagetime = 131072;
	int32_t outstats = 0;
	while (1)
	{
		uint64_t starttime = E32ReadTime();

		// Copy at different lines to make the output appear to scroll
		DMACopyBlocks((uint32_t)(bufferA+offset*W), (uint32_t)bufferB, blockCountInMultiplesOf16bytes);

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

		// Handle Scroll
		offset = (offset+dir);
		if (offset == 0 || offset == H*2-2) dir = -dir;
	}
	return 0;
}
