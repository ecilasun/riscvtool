#include "core.h"
#include "gpu.h"
#include "dma.h"

int main()
{
	const uint32_t W = 320;
	const uint32_t H = 240;
	const uint32_t mode = VIDEOMODE_320PALETTED;

	// Create source and target buffers (using GPU functions to get aligned buffer addresses)
	uint8_t *bufferA = GPUAllocateBuffer(W*H);
	uint8_t *bufferB = GPUAllocateBuffer(W*H);

	// Set buffer B as output
	GPUSetVPage((uint32_t)bufferB);
	GPUSetVMode(MAKEVMODEINFO(mode, VIDEOOUT_ENABLED)); // Mode 0, video on

	// Fill buffer A with some data
	for (uint32_t i=0;i<W*H;++i)
		bufferA[i] = i;

	// Kick a DMA copy to move buffer A contents onto buffer B
	const uint32_t blockCountInMultiplesOf16bytes = (W*H)/16;
	DMACopyBlocks((uint32_t)bufferB, (uint32_t)bufferA, blockCountInMultiplesOf16bytes);

	// Halt the CPU
	while(1) {
		asm volatile("wfi;");
	} while (1);

	return 0;
}
