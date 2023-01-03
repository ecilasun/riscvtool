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
uint32_t prevvblankcount;
#endif

void
I_InitGraphics(void)
{
	usegamma = 1;

	framebuffer = GPUAllocateBuffer(SCREENWIDTH*SCREENHEIGHT);
	memset(framebuffer, 0x0, SCREENWIDTH*SCREENHEIGHT);

	GPUSetVPage((uint32_t)framebuffer);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

#if !defined(FIVEPIPE)
	prevvblankcount = GPUReadVBlankCounter();
#endif
}

void
I_ShutdownGraphics(void)
{
	// TODO: Wait for GPU idle perhaps?
	GPUSetVMode(MAKEVMODEINFO(0, 0)); // Video off
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
	// TODO: Replace with GPU async DMA instead
	// (also won't need cache flush in that case as we don't go through caches)
	memcpy(framebuffer, screens[0], SCREENWIDTH*SCREENHEIGHT);

	// Complete framebuffer writes by invalidating & writing back D$
	asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1
}


void
I_WaitVBL(int count)
{
	// Wait until we exit current frame's vbcounter and enter the next one
#if !defined(FIVEPIPE)
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
