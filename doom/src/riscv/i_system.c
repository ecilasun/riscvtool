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

    if (*BUTTONCHANGEAVAIL)
    {
        uint32_t newbuttons = *BUTTONCHANGE;

		// There's a change if s_buttons ^ newbuttons != 0

		if ((s_buttons ^ newbuttons)&BUTTONMASK_CENTER)
		{
			event.type =  ev_joystick;
			if ((s_buttons&BUTTONMASK_CENTER) == 0) // Old state was 'up', so new event is 'down' etc
				event.data1 = 1;
			else
				event.data1 = 0;
		}

		if ((s_buttons ^ newbuttons)&BUTTONMASK_DOWN)
		{
			if ((s_buttons&BUTTONMASK_DOWN) == 0)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;
			event.data1 = KEY_DOWNARROW;
		}

		if ((s_buttons ^ newbuttons)&BUTTONMASK_LEFT)
		{
			if ((s_buttons&BUTTONMASK_LEFT) == 0)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;
			event.data1 = KEY_LEFTARROW;
		}

		if ((s_buttons ^ newbuttons)&BUTTONMASK_RIGHT)
		{
			if ((s_buttons&BUTTONMASK_RIGHT) == 0)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;
			event.data1 = KEY_RIGHTARROW;
		}

		if ((s_buttons ^ newbuttons)&BUTTONMASK_UP)
		{
			if ((s_buttons&BUTTONMASK_UP) == 0)
				event.type = ev_keydown;
			else
				event.type = ev_keyup;
			event.data1 = KEY_UPARROW;
		}

		s_buttons = newbuttons;
		D_PostEvent(&event);
	}

	// Works with riscvtool's keyserver
	/*unsigned char keydata[2];
	if (*IO_UARTRXByteAvailable)
	{
		for (int i=0;i<2;++i)
			keydata[i] = *IO_UARTRXTX;
		// first byte: 1:down 2:up
		// second byte: x11 keysymbol

		// The packet contains up/down state in first byte
		//event.type = keydata[0] ? ev_keydown : ev_keyup;

		switch (keydata[1])
		{
			case 'w':
				event.data1 = KEY_UPARROW;
			break;

			case 's':
				event.data1 = KEY_DOWNARROW;
			break;

			case 'q':
				event.data1 = ',';
			break;

			case 'e':
				event.data1 = '.';
			break;

			case 'a':
				event.data1 = KEY_LEFTARROW;
			break;

			case 'd':
				event.data1 = KEY_RIGHTARROW;
			break;

			case '0':
				event.type =  ev_joystick;
				event.data1 = keydata[0];
				event.data2 = 0;
				event.data3 = 0;
			break;

			default:
				event.data1 = keydata[1];
			break;
		}

		D_PostEvent(&event);
	}*/

	/*static uint32_t oldhardwareswitchstates = 0;
	if (*IO_SwitchByteCount)
	{
		uint32_t hardwareswitchstates = *IO_SwitchState;

		uint32_t deltastate = hardwareswitchstates ^ oldhardwareswitchstates;

		if (deltastate&0x01)
		{
			event.type =  ev_joystick;
			event.data1 = 0; // buttons (1: fire)
			event.data2 = 0;
			event.data3 = (hardwareswitchstates&0x01) ? 3 : 0;
			D_PostEvent(&event);
		}

		if ((deltastate&0x02))
		{
			event.type =  ev_joystick;
			event.data1 = 0; // buttons
			event.data2 = (hardwareswitchstates&0x02) ? 3 : 0;
			event.data3 = 0;
			D_PostEvent(&event);
		}

		if ((deltastate&0x04))
		{
			event.type = ev_joystick;
			event.data1 = 0; // buttons
			event.data2 = (hardwareswitchstates&0x04) ? -3 : 0;
			event.data3 = 0;
			D_PostEvent(&event);
		}

		if (deltastate&0x08)
		{
			event.type =  ev_joystick;
			event.data1 = 0; // buttons
			event.data2 = 0;
			event.data3 = (hardwareswitchstates&0x08) ? -3 : 0;
			D_PostEvent(&event);
		}

		oldhardwareswitchstates = hardwareswitchstates;
	}*/
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
