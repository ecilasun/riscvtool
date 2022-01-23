/*
 * i_system.c
 *
 * System support code
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../doomdef.h"
#include "../doomstat.h"

#include "../d_main.h"
#include "../g_game.h"
#include "../m_misc.h"
#include "../i_sound.h"
#include "../i_video.h"

#include "../i_system.h"

#include <inttypes.h>
#include "core.h"
#include "console.h"
#include "uart.h"
#include "buttons.h"
#include "ringbuffer.h"

uint8_t *keyboardringbuffer = (uint8_t*)0x80010000;

static uint32_t s_buttons = 0;

void
I_Init(void)
{
	//I_InitSound();
	s_buttons = *BUTTONIMMEDIATESTATE;
}


byte *
I_ZoneBase(int *size)
{
	/* Give 24M to DOOM */
	*size = 24 * 1024 * 1024;
	return (byte *) malloc (*size);
}

// returns time in 1/TICRATEth second tics
int
I_GetTime(void)
{
	static int basetime = 0;
	if (!basetime)
		basetime = E32ReadTime();
	int ticks = TICRATE*ClockToUs(E32ReadTime()-basetime)/1000000;
	return ticks;
}


static void
I_GetEvent(void)
{
	event_t event;

	// Any pending keyboard events to handle?
	uint32_t val;
	swap_csr(mie, MIP_MSIP | MIP_MTIP);
	int R = RingBufferRead(keyboardringbuffer, (uint8_t*)&val, 4);
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);
	if (R)
	{
		uint32_t key = val&0xFF;

		event.type = val&0x100 ? ev_keyup : ev_keydown;
		switch(key)
		{
			case 0x74: event.data1 = KEY_RIGHTARROW; break;
			case 0x6B: event.data1 = KEY_LEFTARROW; break;
			case 0x75: event.data1 = KEY_UPARROW; break;
			case 0x72: event.data1 = KEY_DOWNARROW; break;
			case 0x76: event.data1 = KEY_ESCAPE; break;
			case 0x5A: event.data1 = KEY_ENTER; break;
			case 0x0D: event.data1 = KEY_TAB; break;
			case 0x05: event.data1 = KEY_F1; break;
			case 0x06: event.data1 = KEY_F2; break;
			case 0x04: event.data1 = KEY_F3; break;
			case 0x0C: event.data1 = KEY_F4; break;
			case 0x03: event.data1 = KEY_F5; break;
			case 0x0B: event.data1 = KEY_F6; break;
			case 0x83: event.data1 = KEY_F7; break;
			case 0x0A: event.data1 = KEY_F8; break;
			case 0x01: event.data1 = KEY_F9; break;
			case 0x09: event.data1 = KEY_F10; break;
			case 0x78: event.data1 = KEY_F11; break;
			case 0x07: event.data1 = KEY_F12; break;
			case 0x66: event.data1 = KEY_BACKSPACE; break;
			//case 0x00: event.data1 = KEY_PAUSE; ? break;
			case 0x55: event.data1 = KEY_EQUALS; break;
			case 0x4E: event.data1 = KEY_MINUS; break;
			case 0x59: event.data1 = KEY_RSHIFT; break;
			case 0x14: event.data1 = KEY_RCTRL; break;
			//case 0x11: event.data1 = KEY_RALT; break; // 0xE0+0x11
			case 0x11: event.data1 = KEY_LALT; break;
		}

		if (key == 0x4A) // '/?' key to fire
		{
			event.type =  ev_joystick;
			event.data1 = val&0x100 ?1:0;
		}

		// Debug
		/*UARTWrite(event.type == ev_joystick ? "btn" : (event.type == ev_keyup ? "up":"down"));
		UARTWriteHex(event.data1);
		UARTWrite("\n");*/

		D_PostEvent(&event);
	}
}

void
I_StartFrame(void)
{
	/* Nothing to do */
}

void
I_StartTic(void)
{
	I_GetEvent();
}

ticcmd_t *
I_BaseTiccmd(void)
{
	static ticcmd_t emptycmd;
	return &emptycmd;
}


void
I_Quit(void)
{
	D_QuitNetGame();
	//I_ShutdownSound();
	//I_ShutdownMusic();
	M_SaveDefaults();
	I_ShutdownGraphics();
	exit(0); // NOTE: The environment we're going to return to has been destroyed
}


byte *
I_AllocLow(int length)
{
	byte*	mem;
	mem = (byte *)malloc (length);
	memset (mem,0,length);
	return mem;
}


void
I_Tactile
( int on,
  int off,
  int total )
{
	// UNUSED.
	on = off = total = 0;
}


void
I_Error(char *error, ...)
{
	va_list	argptr;

	// Message first.
	va_start (argptr,error);
	printf ("Error: ");
	vprintf (error, argptr);
	printf ("\n");
	va_end (argptr);

	fflush( stdout );

	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame ();
	I_ShutdownGraphics();

	exit(-1);
}
