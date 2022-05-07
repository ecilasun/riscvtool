/*
 * i_sound.c
 *
 * Dummy sound code
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

#include "../z_zone.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../doomdef.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "leds.h"
#include "apu.h"
#include "core.h"
#include "uart.h"

#define SAMPLECOUNT             512
#define NUM_CHANNELS            8
#define BUFMUL                  4
#define MIXBUFFERSIZE           (SAMPLECOUNT*BUFMUL)
//#define SAMPLERATE              11025   // Hz
//#define SAMPLESIZE              2       // 16bit

signed short mixbuffer[MIXBUFFERSIZE];
unsigned int    channelstep[NUM_CHANNELS];
unsigned int    channelstepremainder[NUM_CHANNELS];
unsigned char*  channels[NUM_CHANNELS];
unsigned char*  channelsend[NUM_CHANNELS];
int             channelstart[NUM_CHANNELS];
int             channelhandles[NUM_CHANNELS];
int             channelids[NUM_CHANNELS];
int             steptable[256];
int             vol_lookup[128*256];
int*            channelleftvol_lookup[NUM_CHANNELS];
int*            channelrightvol_lookup[NUM_CHANNELS];

int lengths[NUMSFX];

#ifdef SNDINTR
uint32_t audioworker(const uint32_t hartid)
{
	// Wait for trigger
	while (1)
	{
		uint32_t trigger = HARTMAILBOX[4*HARTPARAMCOUNT+0+NUM_HARTS];
		if (trigger != 0)
		{
			HARTMAILBOX[4*HARTPARAMCOUNT+0+NUM_HARTS] = 0;

			SetLEDState(0x1); // Busy

			// Make sure we'll re-load from RAM for an accurate image of what's written by HART#0
			asm volatile( ".word 0xFC200073;" ); // CDISCARD.D.L1 (invalidate D$)

			uint32_t *src = (uint32_t *)mixbuffer;

			for (uint32_t i=0; i<SAMPLECOUNT; ++i)
				*APUOUTPUT = src[i];

			SetLEDState(0x0); // Done
		}
	}

	return 1; // Keep alive
}
#endif

//
// This function loads the sound data from the WAD lump,
//  for single sound.
//
void*
getsfx
( char*         sfxname,
  int*          len )
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;


    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    // Now, there is a severe problem with the
    //  sound handling, in it is not (yet/anymore)
    //  gamemode aware. That means, sounds from
    //  DOOM II will be requested even with DOOM
    //  shareware.
    // The sound list is wired into sounds.c,
    //  which sets the external variable.
    // I do not do runtime patches to that
    //  variable. Instead, we will use a
    //  default sound for replacement.
    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);

    size = W_LumpLength( sfxlump );

    // Debug.
    // fprintf( stderr, "." );
    //fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //       sfxname, sfxlump, size );
    //fflush( stderr );

    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(  paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free( sfx );

    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
}

//
// This function adds a sound to the
//  list of currently active sounds,
//  which is maintained as a given number
//  (eight, usually) of internal channels.
// Returns a handle.
//
int
addsfx
( int           sfxid,
  int           volume,
  int           step,
  int           seperation )
{
    static unsigned short       handlenums = 0;

	//printf( "I_StartSound -> addsfx %d\n", sfxid);

    int         i;
    int         rc = -1;

    int         oldest = gametic;
    int         oldestnum = 0;
    int         slot;

    int         rightvol;
    int         leftvol;

    // Chainsaw troubles.
    // Play these sound effects only one at a time.
    if ( sfxid == sfx_sawup
         || sfxid == sfx_sawidl
         || sfxid == sfx_sawful
         || sfxid == sfx_sawhit
         || sfxid == sfx_stnmov
         || sfxid == sfx_pistol  )
    {
        // Loop all channels, check.
        for (i=0 ; i<NUM_CHANNELS ; i++)
        {
            // Active, and using the same SFX?
            if ( (channels[i])
                 && (channelids[i] == sfxid) )
            {
                // Reset.
                channels[i] = 0;
                // We are sure that iff,
                //  there will only be one.
                break;
            }
        }
    }

    // Loop all channels to find oldest SFX.
    for (i=0; (i<NUM_CHANNELS) && (channels[i]); i++)
    {
        if (channelstart[i] < oldest)
        {
            oldestnum = i;
            oldest = channelstart[i];
        }
    }

    // Tales from the cryptic.
    // If we found a channel, fine.
    // If not, we simply overwrite the first one, 0.
    // Probably only happens at startup.
    if (i == NUM_CHANNELS)
        slot = oldestnum;
    else
        slot = i;

    // Okay, in the less recent channel,
    //  we will handle the new SFX.
    // Set pointer to raw data.
    channels[slot] = (unsigned char *) S_sfx[sfxid].data;
    // Set pointer to end of raw data.
    channelsend[slot] = channels[slot] + lengths[sfxid];

    // Reset current handle number, limited to 0..100.
    if (!handlenums)
        handlenums = 100;

    // Assign current handle number.
    // Preserved so sounds could be stopped (unused).
    channelhandles[slot] = rc = handlenums++;

    // Set stepping???
    // Kinda getting the impression this is never used.
    channelstep[slot] = step;
    // ???
    channelstepremainder[slot] = 0;
    // Should be gametic, I presume.
    channelstart[slot] = gametic;

    // Separation, that is, orientation/stereo.
    //  range is: 1 - 256
    seperation += 1;

    // Per left/right channel.
    //  x^2 seperation,
    //  adjust volume properly.
    leftvol =
        volume - ((volume*seperation*seperation) >> 16); ///(256*256);
    seperation = seperation - 257;
    rightvol =
        volume - ((volume*seperation*seperation) >> 16);

    // Sanity check, clamp volume.
    if (rightvol < 0 || rightvol > 127)
        I_Error("rightvol out of bounds");

    if (leftvol < 0 || leftvol > 127)
        I_Error("leftvol out of bounds");

    // Get the proper lookup table piece
    //  for this volume level???
    channelleftvol_lookup[slot] = &vol_lookup[leftvol*256];
    channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

    // Preserve sound SFX id,
    //  e.g. for avoiding duplicates of chainsaw.
    channelids[slot] = sfxid;

    // You tell me.
    return rc;
}

/* Sound */
/* ----- */

void
I_InitSound()
{
	int i;
	for (i=1 ; i<NUMSFX ; i++)
	{
		// Alias? Example is the chaingun sound linked to pistol.
		if (!S_sfx[i].link)
		{
			// Load data from WAD file.
			S_sfx[i].data = getsfx( S_sfx[i].name, &lengths[i] );
		}
		else
		{
			// Previously loaded already?
			S_sfx[i].data = S_sfx[i].link->data;
			lengths[i] = lengths[(S_sfx[i].link - S_sfx)/sizeof(sfxinfo_t)];
		}
	}

	printf("Pre-cached all sound data\n");

#ifdef SNDINTR
	printf("I_InitSound installing TISR\n");
	// TISR Repeats every 1 ms (1000 times per second)
	InstallTimerISR(4, audioworker, ONE_MILLISECOND_IN_TICKS);
	// Wait until ackknowledged by the HART
	while(HARTMAILBOX[4] != 0x0) asm volatile("nop;");
#endif
}

void
I_UpdateSound(void)
{
  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned int sample;
  register int          dl;
  register int          dr;

  // Pointers in global mixbuffer, left, right, end.
  signed short*         leftout;
  signed short*         rightout;
  signed short*         leftend;
  // Step in mixbuffer, left and right, thus two.
  int                           step;

  // Mixing channel index.
  int                           chan;

    // Left and right channel
    //  are in global mixbuffer, alternating.
    leftout = mixbuffer;
    rightout = mixbuffer+1;
    step = 2;

    // Determine end, for left channel only
    //  (right channel is implicit).
    leftend = mixbuffer + SAMPLECOUNT*step;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
    while (leftout != leftend)
    {
        // Reset left/right value.
        dl = 0;
        dr = 0;

        // Love thy L2 chache - made this a loop.
        // Now more channels could be set at compile time
        //  as well. Thus loop those  channels.
        for ( chan = 0; chan < NUM_CHANNELS; chan++ )
        {
            // Check channel, if active.
            if (channels[ chan ])
            {
                // Get the raw data from the channel.
                sample = *channels[ chan ];
                // Add left and right part
                //  for this channel (sound)
                //  to the current data.
                // Adjust volume accordingly.
                dl += channelleftvol_lookup[ chan ][sample];
                dr += channelrightvol_lookup[ chan ][sample];
                // Increment index ???
                channelstepremainder[ chan ] += channelstep[ chan ];
                // MSB is next sample???
                channels[ chan ] += channelstepremainder[ chan ] >> 16;
                // Limit to LSB???
                channelstepremainder[ chan ] &= 65536-1;

                // Check whether we are done.
                if (channels[ chan ] >= channelsend[ chan ])
                    channels[ chan ] = 0;
            }
        }

        // Clamp to range. Left hardware channel.
        // Has been char instead of short.
        // if (dl > 127) *leftout = 127;
        // else if (dl < -128) *leftout = -128;
        // else *leftout = dl;

        if (dl > 0x7fff)
            *leftout = 0x7fff;
        else if (dl < -0x8000)
            *leftout = -0x8000;
        else
            *leftout = dl;

        // Same for right hardware channel.
        if (dr > 0x7fff)
            *rightout = 0x7fff;
        else if (dr < -0x8000)
            *rightout = -0x8000;
        else
            *rightout = dr;

        // Increment current pointers in mixbuffer.
        leftout += step;
        rightout += step;
    }

#ifdef SNDINTR
	// Make sure this data makes it to RAM
	asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1 (writeback D$)
	// Trigger worker thread
	HARTMAILBOX[4*HARTPARAMCOUNT+0+NUM_HARTS] = 0xFFFFFFFF;
#endif
}

void
I_SubmitSound(void)
{
	uint32_t *src = (uint32_t *)mixbuffer;
	for (uint32_t i=0; i<SAMPLECOUNT; ++i)
		*APUOUTPUT = src[i];
}

void
I_ShutdownSound(void)
{
  // TODO: Wait till all pending sounds are finished.
}

void I_SetChannels(void)
{
  // Init internal lookups (raw data, mixing buffer, channels).
  // This function sets up internal lookups used during
  //  the mixing process.
  int           i;
  int           j;

  int*  steptablemid = steptable + 128;

  // Okay, reset internal mixing channels to zero.
  /*for (i=0; i<NUM_CHANNELS; i++)
  {
    channels[i] = 0;
  }*/

  // This table provides step widths for pitch parameters.
  // I fail to see that this is currently used.
  for (i=-128 ; i<128 ; i++)
    steptablemid[i] = (int)(powf(2.f, (i/64.f))*65536.f);


  // Generates volume lookup tables
  //  which also turn the unsigned samples
  //  into signed samples.
  for (i=0 ; i<128 ; i++)
    for (j=0 ; j<256 ; j++)
      vol_lookup[i*256+j] = (i*(j-128)*256)/127;
}

int
I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}

int
I_StartSound
( int id,
  int vol,
  int sep,
  int pitch,
  int priority )
{
	priority = 0;

    id = addsfx( id, vol, steptable[pitch], sep );

	return id;
}

void
I_StopSound(int handle)
{
  handle = 0;
}

int
I_SoundIsPlaying(int handle)
{
    // Ouch.
    return gametic < handle;
}

void
I_UpdateSoundParams
( int handle,
  int vol,
  int sep,
  int pitch )
{
	handle = vol = sep = pitch = 0;
}


/* Music */
/* ----- */

void
I_InitMusic(void)
{
}

void
I_ShutdownMusic(void)
{
}

void I_SetSfxVolume(int volume)
{
  // Identical to DOS.
  // Basically, this should propagate
  //  the menu/config file setting
  //  to the state variable used in
  //  the mixing.
  snd_SfxVolume = volume;
}

// MUSIC API - dummy. Some code from DOS version.
void I_SetMusicVolume(int volume)
{
  // Internal state variable.
  snd_MusicVolume = volume;
  // Now set volume on output device.
  // Whatever( snd_MusciVolume );
}

void
I_PauseSong(int handle)
{
}

void
I_ResumeSong(int handle)
{
}

int
I_RegisterSong(void *data)
{
	return 0;
}

void
I_PlaySong
( int handle,
  int looping )
{
}

void
I_StopSong(int handle)
{
}

void
I_UnRegisterSong(int handle)
{
}
