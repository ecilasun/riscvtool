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

#include "core.h"
#include "gpu.h"

uint8_t *framebuffer;
#if !defined(FIVEPIPE)
#include "dma.h"
uint32_t prevvblankcount;
#endif

void
I_InitGraphics(void)
{
	usegamma = 1;

	// Allocate 40 pixel more since out video output is 240 vs the default 200 pixels here
	framebuffer = GPUAllocateBuffer(SCREENWIDTH*(SCREENHEIGHT+40));
	memset(framebuffer, 0x0, SCREENWIDTH*(SCREENHEIGHT+40));

	GPUSetVPage((uint32_t)framebuffer);
	GPUSetVMode(MAKEVMODEINFO(VIDEOMODE_320PALETTED, VIDEOOUT_ENABLED)); // Mode 0 (320x240x8bitpalette), video output on

#if !defined(FIVEPIPE)
	prevvblankcount = GPUReadVBlankCounter();
#endif
}

void
I_ShutdownGraphics(void)
{
	GPUSetVMode(MAKEVMODEINFO(VIDEOMODE_320PALETTED, VIDEOOUT_DISABLED)); // Video scanout off
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
		GPUSetPal(i, MAKECOLORRGB24(r, g, b));
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
#if defined(FIVEPIPE)
	// CPU handles the copy operation
	memcpy(framebuffer, screens[0], SCREENWIDTH*SCREENHEIGHT);
	// Complete framebuffer writes by invalidating & writing back D$
	CFLUSH_D_L1;
#else

	// Make sure writes to screen[0] made it to RAM before we kick the DMA
	CFLUSH_D_L1;

	// Wait for prior DMA to avoid lock-up
	while (DMAPending()) { asm volatile("nop;"); }

	// DMA controller handles the DMA operation in 128bit (16 byte) blocks.
	// Writes go directly to memory without polluting the data cache
	// but RAM contents have to be recent, hence the flush above.
	uint32_t blockCount = SCREENWIDTH * SCREENHEIGHT / DMA_BLOCK_SIZE;
	DMACopyBlocks((uint32_t)screens[0], (uint32_t)framebuffer, blockCount);
#endif
}


void
I_WaitVBL(int count)
{
#if !defined(FIVEPIPE)
	// Wait until we exit current frame's vbcounter and enter the next one
	while (prevvblankcount == GPUReadVBlankCounter()) { asm volatile ("nop;"); }
	prevvblankcount = GPUReadVBlankCounter();
#endif
}

void
I_ReadScreen(byte* scr)
{
	// Copy what's on screen
	memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}
