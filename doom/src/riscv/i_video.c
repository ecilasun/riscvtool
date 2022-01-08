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

//static uint32_t vramPage = 0;

void flippage()
{
	//
}

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


volatile uint32_t *GPUPAL_32 = (volatile uint32_t* )0x40040000;
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g)<<16) | ((_r)<<8) | (_b))

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
	// Fake screen buffer in BRAM (overwrites loader in BIOS)
	// TODO: Somehow make sure screens[0] is within BRAM region instead of DDR3
	for (int slice = 0; slice<4; ++slice)
	{
		int H=64;
		if (slice==4)
			H = 16;

		// The out buffer needs a stride of 512 instead of 320
		uint32_t GRAMStart = 0x40000000;
		for (int L=0;L<H;++L)
			__builtin_memcpy((void*)GRAMStart+512*L, screens[0]+SCREENWIDTH*64*slice+SCREENWIDTH*L, 320);
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
