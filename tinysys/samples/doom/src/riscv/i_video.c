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
#include <stdlib.h>

#include "../doomdef.h"

#include "../i_system.h"
#include "../v_video.h"
#include "../i_video.h"

#include "basesystem.h"
#include "core.h"
#include "gpu.h"

uint8_t *framebuffer;
#include "dma.h"
uint32_t prevvblankcount;
struct EVideoContext g_vctx;

void
I_InitGraphics(void)
{
	usegamma = 1;

	// Allocate 40 pixel more since out video output is 240 vs the default 200 pixels here
	framebuffer = GPUAllocateBuffer(SCREENWIDTH*(SCREENHEIGHT+40));
	memset(framebuffer, 0x0, SCREENWIDTH*(SCREENHEIGHT+40));

	GPUSetVMode(&g_vctx, EVM_320_Pal, EVS_Enable);
	GPUSetWriteAddress(&g_vctx, (uint32_t)framebuffer);
	GPUSetScanoutAddress(&g_vctx, (uint32_t)framebuffer);

	prevvblankcount = GPUReadVBlankCounter();
}

void
I_ShutdownGraphics(void)
{
	GPUSetVMode(&g_vctx, EVM_320_Pal, EVS_Disable);
}

void
I_SetPalette(byte* palette)
{
	// Copy palette to G-RAM
	byte r, g, b;
	for (int i=0 ; i<256 ; i++) {
		r = gammatable[usegamma][*palette++];
		g = gammatable[usegamma][*palette++];
		b = gammatable[usegamma][*palette++];
		GPUSetPal(i, r, g, b);
	}
}


void
I_UpdateNoBlit(void)
{
	// hmm....
}

void
I_FinishUpdate (void)
{
	// Option 1: CPU handles the copy operation
	// memcpy(framebuffer, screens[0], SCREENWIDTH*SCREENHEIGHT);

	// In either case, write pending data to memory to ensure all writes are visible to scan-out or DMA
	CFLUSH_D_L1;

	// Option 2: DMA unit handles the copy operation
	// Wait for prior DMA to avoid lock-up
	while (DMAPending()) { asm volatile("nop;"); }

	// DMA controller handles the DMA operation in 128bit (16 byte) blocks.
	// Writes go directly to memory without polluting the data cache
	// but RAM contents have to be recent, hence the flush above.
	uint32_t blockCount = SCREENWIDTH * SCREENHEIGHT / DMA_BLOCK_SIZE;
	const uint32_t leftoverDMA = blockCount%256;
	const uint32_t fullDMAs = blockCount/256;

	// Do the full DMAs first
	uint32_t fulloffset = 0;
	for (uint32_t full=0;full<fullDMAs;++full)
	{
		DMACopy4K((uint32_t)screens[0]+fulloffset, (uint32_t)framebuffer+fulloffset);
		fulloffset += 256*16; // 16 bytes for each 256-block, total of 4K
	}

	// Partial DMA next
	if (leftoverDMA!=0)
		DMACopy((uint32_t)screens[0]+fulloffset, (uint32_t)framebuffer+fulloffset, (uint8_t)(leftoverDMA-1));

	// Tag for sync (essentially an item in FIFO after last DMA so we can check if DMA is complete when this drains)
	DMATag(0x0);

	//DMACopyBlocks((uint32_t)screens[0], (uint32_t)framebuffer, blockCount);
}


void
I_WaitVBL(int count)
{
	// Wait until we exit current frame's vbcounter and enter the next one
	while (prevvblankcount == GPUReadVBlankCounter()) { asm volatile ("nop;"); }
	prevvblankcount = GPUReadVBlankCounter();
}

void
I_ReadScreen(byte* scr)
{
	// Copy what's on screen
	memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}
