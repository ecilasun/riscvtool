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

#include "core.h"
#include "gpu.h"

void
I_InitGraphics(void)
{
	usegamma = 1;

	uint32_t addrs = (uint32_t)(screens[0]);
	GPUSetVPage(addrs);

	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on
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
	// This is where we're supposed to flip to the next video page

	//uint32_t *VID = (uint32_t *)screens[0];
	//GPUSetVPage(VID);
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
