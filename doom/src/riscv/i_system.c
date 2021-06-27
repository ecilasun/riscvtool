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
#include "console.h"
#include "config.h"

void
I_Init(void)
{
	//I_InitSound();
}


byte *
I_ZoneBase(int *size)
{
	/* Give 12M to DOOM */
	*size = 12 * 1024 * 1024;
	return (byte *) malloc (*size);
}

// returns time in 1/TICRATEth second tics
int
I_GetTime(void)
{
	static int basetime = 0;
	if (!basetime)
		basetime = ReadClock();
	int ticks = TICRATE*ClockToUs(ReadClock()-basetime)/1000000;
	return ticks;
}


static void
I_GetEvent(void)
{
	static uint8_t prevchar = 0;

	event_t event;

	if (*IO_UARTRXByteCount)
	{
		uint8_t inchar = *IO_UARTRX;

		// For each character that was received,
		// create a 'down' event, and an 'up' event
		// for the one before it if they're different
		event.type = ev_keydown;

		switch (inchar)
		{
			case 'w':
				event.data1 = KEY_UPARROW;
				prevchar = KEY_UPARROW;
			break;

			case 's':
				event.data1 = KEY_DOWNARROW;
				prevchar = KEY_DOWNARROW;
			break;

			case 'q':
				event.data1 = KEY_LEFTARROW;
				prevchar = KEY_LEFTARROW;
			break;

			case 'e':
				event.data1 = KEY_RIGHTARROW;
				prevchar = KEY_RIGHTARROW;
			break;

			case 'a':
				event.data1 = ',';
				prevchar = ',';
			break;

			case 'd':
				event.data1 = '.';
				prevchar = '.';
			break;

			default:
				event.data1 = inchar;
				prevchar = inchar;
			break;
		}

		D_PostEvent(&event);
	}
	else
	{
		// Fake an 'up' event for the
		// last known key, if there was one
		if (prevchar != 0)
		{
			event.type = ev_keyup;
			event.data1 = prevchar;
			D_PostEvent(&event);
			prevchar = 0;
		}
	}

	static uint32_t oldhardwareswitchstates = 0;
	if (*IO_SwitchByteCount)
	{
		uint32_t hardwareswitchstates = *IO_SwitchState;

		uint32_t deltastate = hardwareswitchstates ^ oldhardwareswitchstates;

		/*if (deltastate&0x01)
		{
			event.type = (hardwareswitchstates&0x01) ? ev_keydown : ev_keyup;
			event.data1 = 'w';
			D_PostEvent(&event);
		}

		if (deltastate&0x02)
		{
			event.type = (hardwareswitchstates&0x02) ? ev_keydown : ev_keyup;
			event.data1 = 's';
			D_PostEvent(&event);
		}

		if (deltastate&0x04)
		{
			event.type = (hardwareswitchstates&0x04) ? ev_keydown : ev_keyup;
			event.data1 = 'd';
			D_PostEvent(&event);
		}

		if (deltastate&0x08)
		{
			event.type = (hardwareswitchstates&0x08) ? ev_keydown : ev_keyup;
			event.data1 = 'a';
			D_PostEvent(&event);
		}*/

		if (deltastate&0x10)
		{
			event.type =  ev_joystick;
			event.data1 = (hardwareswitchstates&0x10) ? 1:0;
			event.data2 = 0;
			event.data3 = 0;
			D_PostEvent(&event);
		}

		if ((deltastate&0x20))
		{
			event.type =  ev_joystick;
			event.data1 = 0; // buttons
			event.data2 = (hardwareswitchstates&0x20) ? 3 : 0;
			event.data3 = 0;
			D_PostEvent(&event);
		}

		if ((deltastate&0x40))
		{
			event.type = ev_joystick;
			event.data1 = 0; // buttons
			event.data2 = (hardwareswitchstates&0x40) ? -3 : 0;
			event.data3 = 0;
			D_PostEvent(&event);
		}

		if (deltastate&0x80)
		{
			// SDCARD
		}

		oldhardwareswitchstates = hardwareswitchstates;
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
