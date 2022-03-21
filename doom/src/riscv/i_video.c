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

static uint32_t writepage = 0;

void
I_InitGraphics(void)
{
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
	// Copy palette to G-RAM
	byte r, g, b;
	for (int i=0 ; i<256 ; i++) {
		r = gammatable[usegamma][*palette++];
		g = gammatable[usegamma][*palette++];
		b = gammatable[usegamma][*palette++];
		GPUPAL_32[i] = MAKERGBPALETTECOLOR(r, g, b);
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
	// NOTE: This is unnecessary, E32E has direct scan-out buffers that the CPU can write to
	// with built-in double-buffering, therefore this copy is redundant.
	// We set up screen[0] to point at GPUFB0 in v_video.c to avoid the following.

	// This is here just to be 'correct' and not performant
	for (int L=0;L<SCREENHEIGHT;++L)
		__builtin_memcpy((void*)GPUFB+SCREENWIDTH*L, screens[0]+SCREENWIDTH*L, SCREENWIDTH);

	// Swap to the next write page
	// This will switch the scanout hardware to the page we're not writing to anymore (i.e. Flip())
	writepage++;
	*GPUCTL = (writepage%2);
}


void
I_WaitVBL(int count)
{
	//GPUWaitForVsync(); <= This is to make GPU wait for vblank, not the CPU
	// Correct approach is to wait for the gpustate write from GPU side at end of frame (GPU label write to mark frame complete)
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
