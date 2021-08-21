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

void flippage()
{
	vramPage = (vramPage+1)%2;
	GPUSetRegister(2, vramPage);
	GPUSetVideoPage(2);
}

void
I_InitGraphics(void)
{
	//*gpustate = 0;

	GPUSetRegister(2, vramPage);
	GPUSetVideoPage(2);

	// Clear one of the pages to color palette entry at 0xFF
	// so that we may see the rate at which the game renders
	// when using debug mode to draw less than full height.
    //GPUSetRegister(1, 0xFFFFFFFF);
    //GPUClearVRAMPage(1);

	// Show the result
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
	byte r, g, b;

	for (int i=0 ; i<256 ; i++) {
		r = gammatable[usegamma][*palette++];
		g = gammatable[usegamma][*palette++];
		b = gammatable[usegamma][*palette++];
		GPUSetRegister(0, i);
    	GPUSetRegister(1, ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b);
	    GPUSetPaletteEntry(0, 1);
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
	/* Copy from RAM buffer to frame buffer */

	// NOTE: Cannot DMA from extended memory (DDR3)
	// So we need to either draw pixel by pixel or copy to BRAM first, then DMA out

	/*{
		static int lasttic = 0;
       	int i = I_GetTime();
        int tics = i - lasttic;
        lasttic = i;
        if (tics > 20) tics = 20;

        for (i=0 ; i<tics*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-128)*SCREENWIDTH + i] = 0xff;
        for ( ; i<20*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-128)*SCREENWIDTH + i] = 0x0;
	}*/

	//if (*gpustate == cnt)
	{
		//++cnt;

		// Two buffers
		uint8_t *g_ram_videobuffers[2] = {(uint8_t*)0x10000000, (uint8_t*)0x10002000}; // Two 8K slices from top of GRAM (512*16 pixels each)

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

			// Single DMA because backbuffer is same size as display
			uint32_t sysramsource = (uint32_t)g_ram_videobuffers[vB];
			GPUSetRegister(4, sysramsource);
			uint32_t vramramtarget = (512*16*slice)>>2;
			GPUSetRegister(5, vramramtarget);
			uint32_t dmacount = (512*H)>>2; //2048 DWORD writes or 1024 for end slice
			GPUKickDMA(4, 5, dmacount, 0);

			// Go to next buffer as the current one is busy copying to the GPU
			vB = (vB+1)%2;
		}

		//GPUWaitForVsync();

		// Show the result
		flippage();

		// GPU status address in G1
		//uint32_t gpustateDWORDaligned = (uint32_t)(gpustate);
		//GPUSetRegister(1, gpustateDWORDaligned);

		// Write 'end of processing' from GPU so that CPU can resume its work
		//GPUSetRegister(2, cnt);
		//GPUWriteSystemMemory(2, 1);

		//*gpustate = 0;
	}
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
