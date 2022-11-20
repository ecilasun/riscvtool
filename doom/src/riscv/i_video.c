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

static uint32_t cycle = 0;

#if defined(FIVEPIPE)
uint8_t *framebufferA;
uint8_t *framebufferB;
uint8_t *readpage;
uint8_t *writepage;
#endif

void
I_InitGraphics(void)
{
	usegamma = 1;

#if defined(FIVEPIPE)
	framebufferA = (uint8_t*)malloc(320*240*3 + 64);
	framebufferB = (uint8_t*)malloc(320*240*3 + 64);
	framebufferA = (uint8_t*)E32AlignUp((uint32_t)framebufferA, 64);
	framebufferB = (uint8_t*)E32AlignUp((uint32_t)framebufferB, 64);

	memset(framebufferA, 0x0, 320*240*3);
	memset(framebufferB, 0x0, 320*240*3);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on
#else

#endif
}

void
I_ShutdownGraphics(void)
{
#if defined(FIVEPIPE)
	// TODO: Wait for GPU idle perhaps?
	GPUSetVMode(MAKEVMODEINFO(0, 0)); // Video off
#else

#endif
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
#if defined(FIVEPIPE)
		GPUSetPal(i, MAKECOLORRGB24(r, g, b));
#else
		GPUPAL_32[i] = MAKERGBPALETTECOLOR(r, g, b);
#endif
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
	// Video scan-out page
	readpage = (cycle%2) ? framebufferA : framebufferB;

	// Video write page
	writepage = (cycle%2) ? framebufferB : framebufferA;

	// Point to new write page (forcing double buffering here)
	screens[0] = writepage;

	// Invalidate & write back D$ (CFLUSH.D.L1)
	asm volatile( ".word 0xFC000073;" );

	// Show the read page while we're writing to the write page
	GPUSetVPage((uint32_t)readpage);

#else
	uint32_t readpage = cycle%2;
	uint32_t writepage = (cycle+1)%2;
	FrameBufferSelect(readpage, writepage);

	screens[0] = (byte*)GPUFB;//writepage == 0 ? (byte*)GPUFB : (byte*)((uint32_t)GPUFB + 320*200*3);
#endif

	// TODO: Might want to move the actual buffer flip to I_WaitVBL()
	++cycle;
}


void
I_WaitVBL(int count)
{
	// Wait for vsync by triggering a write at vsync from GPU side
	// which the CPU can read and compare to an expected value.

	/*GPUWriteWord(frameIndexAddress, nextFrameIndex, GPU_AT_VSYNC); // GPU_AT_VSYNC or GPU_IMMEDIATE
	while (frameIndexAddress != nextFrameIndex) { };
	++nextFrameIndex;*/
}


void
I_ReadScreen(byte* scr)
{
#if defined(FIVEPIPE)
	// Copy what's on screen
	memcpy (scr, readpage, SCREENWIDTH*SCREENHEIGHT);
#else

#endif
}
