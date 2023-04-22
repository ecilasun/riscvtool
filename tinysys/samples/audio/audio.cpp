#include <stdio.h>
#include <math.h>
#include <cstdlib>

#include "core.h"
#include "uart.h"
#include "apu.h"
#include "gpu.h"

static struct EVideoContext s_vx;
static uint8_t *s_framebufferA;
static uint8_t *s_framebufferB;
static short *apubuffer;

#define NUM_CHANNELS 2			// Stereo
#define BUFFER_SAMPLES 512		// buffer size

void DrawWaveform()
{
	static uint32_t cycle = 0;

	// Video scan-out page
	uint8_t *readpage = (cycle%2) ? s_framebufferA : s_framebufferB;
	// Video write page
	uint8_t *writepage = (cycle%2) ? s_framebufferB : s_framebufferA;

	GPUSetWriteAddress(&s_vx, (uint32_t)writepage);
	GPUSetScanoutAddress(&s_vx, (uint32_t)readpage);

	// Clear screen to light magenta
	uint32_t *writepageword = (uint32_t*)writepage;
	for (uint32_t i=0;i<80*240;++i)
		writepageword[i] = 0x50505050;

	int16_t *src = (int16_t *)apubuffer;
	for (uint32_t x=0; x<BUFFER_SAMPLES/2; ++x)
	{
		int16_t L = src[x*2+0];
		int16_t R = src[x*2+1];
		L = L/256;
		L = L < -110 ? -110 : L;
		L = L > 110 ? 110 : L;
		R = R/256;
		R = R < -110 ? -110 : R;
		R = R > 110 ? 110 : R;
		writepage[x + (L+110)*320] = 0x37; // Blue
		writepage[x + (R+110)*320] = 0x28; // Red
	}

    CFLUSH_D_L1;

	++cycle;
}

int main()
{
	s_framebufferB = GPUAllocateBuffer(320*240);
	s_framebufferA = GPUAllocateBuffer(320*240);
	GPUSetVMode(&s_vx, EVM_320_Pal, EVS_Enable);
	GPUSetWriteAddress(&s_vx, (uint32_t)s_framebufferA);
	GPUSetScanoutAddress(&s_vx, (uint32_t)s_framebufferB);
	GPUSetDefaultPalette(&s_vx);

	apubuffer = (short*)APUAllocateBuffer(BUFFER_SAMPLES*NUM_CHANNELS*sizeof(short));
	UARTWrite("APU mix buffer at 0x");
	UARTWriteHex((unsigned int)apubuffer);
	UARTWrite("\n");

	APUSetBufferSize(BUFFER_SAMPLES);
	uint32_t prevframe = APUFrame();

	float offset = 0.f;
	do{
		// Generate a sin and cos wave for each side
		for (uint32_t i=0;i<BUFFER_SAMPLES;++i)
		{
			apubuffer[i*NUM_CHANNELS+0] = short(16384.f*sinf(offset+3.1415927f*float(i)/512.f));
			apubuffer[i*NUM_CHANNELS+1] = short(16384.f*cosf(offset+3.1415927f*float(i)/512.f));
		}

		// Make sure the writes are visible by the DMA
		CFLUSH_D_L1;

		// Wait for the APU to finish playing back current read buffer
		uint32_t currframe;
		do
		{
			currframe = APUFrame();
		} while (currframe == prevframe);
		prevframe = currframe;
		UARTWriteHex((unsigned int)prevframe);
		UARTWrite("\n");

		// Fill current write buffer with new mix data
		APUStartDMA((uint32_t)apubuffer);
		// Read buffer drained, swap to new read buffer
		APUSwapBuffers();

		DrawWaveform();

		offset += 1.f;

	} while(1);

	return 0;
}
