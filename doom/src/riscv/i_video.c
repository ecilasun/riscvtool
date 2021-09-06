/*
 * i_video.c
 *
 * Video system support code
 *
 * Copyright (C) 2021 Sylvain Munaut
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <string.h>

#include "../doomdef.h"

#include "../i_system.h"
#include "../v_video.h"
#include "../i_video.h"

#include "config.h"
#include "gpu.h"

static uint32_t vramPage = 0;

uint8_t video_pal[256];

//volatile uint32_t *gpustate = (volatile uint32_t*)0x1001FFF0; // End of GRAM-16
unsigned int cnt = 0x00000000;

struct GPUCommandPackage swapProg;
struct GPUCommandPackage gpuSetupProg;
struct GPUCommandPackage gpuPaletteProg;
struct GPUCommandPackage dmaProg;

void flippage()
{
	vramPage = (vramPage+1)%2;

	// Wire up a 'swap' program on the fly to set current vram write page
	GPUInitializeCommandPackage(&swapProg);
	GPUWritePrologue(&swapProg);
	GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF(vramPage)));
	GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF(vramPage)));
	GPUWriteInstruction(&swapProg, GPU_INSTRUCTION(G_MISC, G_R15, 0x0, 0x0, G_VPAGE));
	GPUWriteEpilogue(&swapProg);
	GPUCloseCommandPackage(&swapProg);

	GPUClearMailbox();
	GPUSubmitCommands(&swapProg);
	GPUKick();
	GPUWaitMailbox();
}

void
I_InitGraphics(void)
{
	// Setup program doesn't do much at the moment except set v-ram page 0 for writes
    GPUInitializeCommandPackage(&gpuSetupProg);
    GPUWritePrologue(&gpuSetupProg);
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE));
    GPUWriteEpilogue(&gpuSetupProg);
    GPUCloseCommandPackage(&gpuSetupProg);

	GPUClearMailbox();
	GPUSubmitCommands(&gpuSetupProg);
	GPUKick();
	GPUWaitMailbox();

	// Initial V-RAM write page select
	flippage();

	usegamma = 1;
}

void
I_ShutdownGraphics(void)
{
	// TODO: Wait for GPU idle perhaps?
}


void
I_SetPalette(byte* palette)
{
	// Re-use setup program as palette setup
    GPUInitializeCommandPackage(&gpuSetupProg);
    GPUWritePrologue(&gpuSetupProg);
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE)); // Write to page 0
	GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_HIGHBITS, G_R14, 0x0000)); // Upper half of palette index is always 0

    byte r, g, b;
	for (int i=0 ; i<256 ; i++) {
		r = gammatable[usegamma][*palette++];
		g = gammatable[usegamma][*palette++];
		b = gammatable[usegamma][*palette++];
        uint32_t color = MAKERGBPALETTECOLOR(r, g, b);
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF(color)));
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF(color)));
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_LOWBITS, G_R14, LOHALF(i)));
        GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_WPAL, G_R15, G_R14, 0x0, 0x0));
    }
    GPUWriteEpilogue(&gpuSetupProg);
    GPUCloseCommandPackage(&gpuSetupProg);

	GPUClearMailbox();
	GPUSubmitCommands(&gpuSetupProg);
	GPUKick();
	GPUWaitMailbox();
}


void
I_UpdateNoBlit(void)
{
	// hmm....
}

void
I_FinishUpdate (void)
{
	// User two temporary buffers
	uint8_t *g_ram_videobuffers[2] = {(uint8_t*)GRAMStart, (uint8_t*)GRAMStart+0x800}; // Two 8K slices from top of GRAM (512*16 pixels each)

	// Fake screen buffer in BRAM (overwrites loader in BIOS)
	// TODO: Somehow make sure screens[0] is within BRAM region instead of DDR3
	static int vB = 0;
	for (int slice = 0; slice<13; ++slice)
	{
		int H=16;
		if (slice==12)
			H = 8;

		// The out buffer needs a stride of 512 instead of 320
		for (int L=0;L<H;++L)
			__builtin_memcpy((void*)g_ram_videobuffers[vB]+512*L, screens[0]+SCREENWIDTH*16*slice+SCREENWIDTH*L, 320);

		// DMA the long way around
		GPUInitializeCommandPackage(&dmaProg);
		GPUWritePrologue(&dmaProg);
		uint32_t gramsource = (uint32_t)(g_ram_videobuffers[vB]);
		GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, HIHALF((uint32_t)gramsource)));
		GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, LOHALF((uint32_t)gramsource)));    // setregi r15, (uint32_t)mandelbuffer
		uint32_t vramramtarget = (512*16*slice);
		GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_HIGHBITS, G_R14, HIHALF((uint32_t)vramramtarget)));
		GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_SETREG, G_R14, G_LOWBITS, G_R14, LOHALF((uint32_t)vramramtarget)));   // setregi r14, (uint32_t)vramtarget
		uint32_t dmacount = (512*H)>>2; //2048 DWORD writes or 1024 for end slice
		GPUWriteInstruction(&dmaProg, GPU_INSTRUCTION(G_DMA, G_R15, G_R14, 0x0, dmacount));                                   // dma r15, r14, dmacount
		GPUWriteEpilogue(&dmaProg);
		GPUCloseCommandPackage(&dmaProg);

		GPUClearMailbox();
		GPUSubmitCommands(&dmaProg);
		GPUKick();
		GPUWaitMailbox();

		// Go to next buffer as the current one is busy copying to the GPU
		vB = (vB+1)%2;
	}

	// optional: wait for vsync

	// Show the result
	flippage();
}


void
I_WaitVBL(int count)
{
	//GPUWaitForVsync(); <= This is to make GPU wait for vblank, not the CPU
	// Correct approach is to wait for the gpustate write from GPU side at end of frame
}


void
I_ReadScreen(byte* scr)
{
	memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


#if 0
void
I_BeginRead(void)
{
}

void
I_EndRead(void)
{
}
#endif
