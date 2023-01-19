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
	uint8_t *bufferA = GPUAllocateBuffer(W*H);
	uint8_t *bufferB = GPUAllocateBuffer(W*H);

	// Set buffer B as output
	GPUSetVPage((uint32_t)bufferB);
	GPUSetVMode(MAKEVMODEINFO(mode, VIDEOOUT_ENABLED)); // Mode 0, video on

	// Fill buffer A with some data
	for (uint32_t i=0;i<W*H;++i)
		bufferA[i] = i;

	// DMA operatins work directly on memory.
	// Therefore, we need to insert a cache flush here so that
	// the writes to buffer A are all written back to RAM.
	CFLUSH_D_L1;

	// Kick a DMA copy to move buffer A contents onto buffer B
	const uint32_t blockCountInMultiplesOf16bytes = (W*H)/16;
	UARTWrite("Initiating copy of ");
	UARTWriteDecimal(blockCountInMultiplesOf16bytes);
	UARTWrite(" blocks\n");
	DMACopyBlocks((uint32_t)bufferA, (uint32_t)bufferB, blockCountInMultiplesOf16bytes);

	// This is not necessary, but used here as an indicator of copy being asynchronous
	// and that we can still work while it's running.
	while (DMAPending())
		UARTWrite(".");

	UARTWrite("\nCopy is now done\n");
	return 0;
}
