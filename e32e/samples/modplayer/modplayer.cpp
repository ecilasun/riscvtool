#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "gpu.h"
#include "leds.h"
#include "apu.h"
#include "ps2.h"
#include "sdcard.h"
#include "fat32/ff.h"

#include "micromod/micromod.h"

uint8_t *framebufferA = (uint8_t*)malloc(320*240*3 + 64);
uint8_t *framebufferB = (uint8_t*)malloc(320*240*3 + 64);
static uint32_t cycle = 0;

#define MULTICORE

FATFS Fs;

#define SAMPLING_FREQ  48000  /* 48khz. */
#define REVERB_BUF_LEN 1100    /* 12.5ms. */
#define OVERSAMPLE     2      /* 2x oversampling. */
#define NUM_CHANNELS   2      /* Stereo. */
#define BUFFER_SAMPLES 512  /* buffer size */

static short mix_buffer[ BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE ];
static short reverb_buffer[ REVERB_BUF_LEN ];
short buffer[BUFFER_SAMPLES*NUM_CHANNELS];
static long reverb_len, reverb_idx, filt_l, filt_r;
static long samples_remaining;

/*
	2:1 downsampling with simple but effective anti-aliasing.
	Count is the number of stereo samples to process, and must be even.
	input may point to the same buffer as output.
*/
static void downsample( short *input, short *output, long count ) {
	long in_idx, out_idx, out_l, out_r;
	in_idx = out_idx = 0;
	while( out_idx < count ) {	
		out_l = filt_l + ( input[ in_idx++ ] >> 1 );
		out_r = filt_r + ( input[ in_idx++ ] >> 1 );
		filt_l = input[ in_idx++ ] >> 2;
		filt_r = input[ in_idx++ ] >> 2;
		output[ out_idx++ ] = out_l + filt_l;
		output[ out_idx++ ] = out_r + filt_r;
	}
}

/* Simple stereo cross delay with feedback. */
static void reverb( short *buffer, long count ) {
	long buffer_idx, buffer_end;
	if( reverb_len > 2 ) {
		buffer_idx = 0;
		buffer_end = buffer_idx + ( count << 1 );
		while( buffer_idx < buffer_end ) {
			buffer[ buffer_idx ] = ( buffer[ buffer_idx ] * 3 + reverb_buffer[ reverb_idx + 1 ] ) >> 2;
			buffer[ buffer_idx + 1 ] = ( buffer[ buffer_idx + 1 ] * 3 + reverb_buffer[ reverb_idx ] ) >> 2;
			reverb_buffer[ reverb_idx ] = buffer[ buffer_idx ];
			reverb_buffer[ reverb_idx + 1 ] = buffer[ buffer_idx + 1 ];
			reverb_idx += 2;
			if( reverb_idx >= reverb_len ) {
				reverb_idx = 0;
			}
			buffer_idx += 2;
		}
	}
}

/* Reduce stereo-separation of count samples. */
static void crossfeed( short *audio, int count ) {
	int l, r, offset = 0, end = count << 1;
	while( offset < end ) {
		l = audio[ offset ];
		r = audio[ offset + 1 ];
		audio[ offset++ ] = ( l + l + l + r ) >> 2;
		audio[ offset++ ] = ( r + r + r + l ) >> 2;
	}
}

static long read_file( const char *filename, void *buffer, long length ) {
	FILE *file;
	long count;
	count = -1;
	file = fopen( filename, "rb" );
	if( file != NULL ) {
		count = fread( buffer, 1, length, file );
		if( count < length && !feof( file ) ) {
			printf("Unable to read file '%s'.\n", filename );
			count = -1;
		}
		if( fclose( file ) != 0 ) {
			printf("Unable to close file '%s'.\n", filename );
		}
	}
  else
    printf("Unable to find file '%s'.\n", filename );
	return count;
}

static void print_module_info() {
	int inst;
	char string[ 23 ];
	for( inst = 0; inst < 16; inst++ ) {
		micromod_get_string( inst, string );
		printf( "%02i - %-22s ", inst, string );
		micromod_get_string( inst + 16, string );
		printf( "%02i - %-22s\n", inst + 16, string );
	}
}

static long read_module_length( const char *filename ) {
	long length;
	signed char header[ 1084 ];
	length = read_file( filename, header, 1084 );
	if( length == 1084 ) {
		length = micromod_calculate_mod_file_len( header );
		if( length < 0 ) {
			printf("Module file type not recognised.\n");
		}
	} else {
		printf("Unable to read module file '%s'.\n", filename );
		length = -1;
	}
	return length;
}

void DrawWaveform()
{
	// Video scan-out page
	uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;
	// Video write page
	uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;
	// Show the read page while we're writing to the write page
	GPUSetVPage((uint32_t)readpage);

	GPUClearScreen(writepage, VIDEOMODE_320PALETTED, 0x0F0F0F0F); // White
	int16_t *src = (int16_t *)buffer;
	for (uint32_t x=0; x<BUFFER_SAMPLES/2; ++x)
	{
		int16_t L = src[x*2+0];
		int16_t R = src[x*2+1];
		L = L/64;
		L = L < -110 ? -110 : L;
		L = L > 110 ? 110 : L;
		R = R/64;
		R = R < -110 ? -110 : R;
		R = R > 110 ? 110 : R;
		writepage[x + (L+110)*320] = 0x01; // Blue
		writepage[x + (R+110)*320] = 0x04; // Red
	}

    asm volatile( ".word 0xFC000073;");

	++cycle;
}

static long play_module( signed char *module )
{
	long result;

	result = micromod_initialise( module, SAMPLING_FREQ * OVERSAMPLE );
	if( result == 0 )
	{
		print_module_info();
		samples_remaining = micromod_calculate_song_duration();
		printf( "Song Duration: %li seconds.\n", samples_remaining / ( SAMPLING_FREQ * OVERSAMPLE ) );
		fflush( NULL );

		int playing = 1;
		while( playing )
		{
			int count = BUFFER_SAMPLES * OVERSAMPLE;
			if( count > samples_remaining )
				count = samples_remaining;

			// Anything above 19ms stalls this
			__builtin_memset( mix_buffer, 0, BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE * sizeof( short ) );
			micromod_get_audio( mix_buffer, count );
			downsample( mix_buffer, buffer, BUFFER_SAMPLES * OVERSAMPLE );
			crossfeed( buffer, BUFFER_SAMPLES );
			reverb( buffer, BUFFER_SAMPLES );

			samples_remaining -= count;

			// TODO: If 'buffer' is placed in AudioRAM, the APU
			// could kick the synchronized copy so we can free the CPU

			// Audio FIFO will be drained at playback rate and
			// the CPU will stall to wait if the FIFO is full.
			// Therefore, no need to worry about synchronization.
			uint32_t *src = (uint32_t *)buffer;
			for (uint32_t i=0; i<BUFFER_SAMPLES; ++i)
				*APUOUTPUT = src[i];

			// Press down arrow to stop
			/*int stop = 0;
			if (*IO_SWITCHREADY)
			{
				hardwareswitchstates = *IO_SWITCHES;
				switch(hardwareswitchstates ^ oldhardwareswitchstates)
				{
					case 0x02: stop = hardwareswitchstates&0x02 ? 0 : 1; break;
					default: break;
				};
				oldhardwareswitchstates = hardwareswitchstates;
			}
			if (stop) playing = 0;*/

			if( samples_remaining <= 0 || result != 0 )
				playing = 0;

#if defined(MULTICORE)
			const int hartID = 1;
			asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1 (writeback D$)
			HARTMAILBOX[NUM_HARTS+hartID*HARTPARAMCOUNT+0] = 0xFFFFFFFF;
#else
			DrawWaveform();
#endif
		}
	}
	else
		printf("micromod_initialise failed\n");

	return result;
}

void PlayMODFile(const char *fname)
{
	signed char *module;
	long count, length;

	/* Read module file.*/
	length = read_module_length( fname );
	if( length > 0 )
	{
		printf( "Module Data Length: %li bytes.\n", length );
		module = (signed char*)calloc( length, 1 );
		if( module != NULL )
		{
			count = read_file( fname, module, length );
			if( count < length )
				printf("Module file is truncated. %li bytes missing.\n", length - count );
			play_module( module );
			free( module );
		}
	}
}

#if defined(MULTICORE)
void vumeterTISR(const uint32_t hartID)
{
	if (HARTMAILBOX[NUM_HARTS+hartID*HARTPARAMCOUNT+0] != 0x0)
	{
		HARTMAILBOX[NUM_HARTS+hartID*HARTPARAMCOUNT+0] = 0x0;

		LEDSetState(0x1); // Busy
		DrawWaveform();
		LEDSetState(0x0); // Not busy
	}

	// Keep alive (zero to terminate)
	HARTMAILBOX[hartID*HARTPARAMCOUNT+0+NUM_HARTS] = 1;
}
#endif

int main()
{
	framebufferA = GPUAllocateBuffer(320*240*3);
	framebufferB = GPUAllocateBuffer(320*240*3);
	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(0, 1)); // Mode 0, video on

	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt != FR_OK)
		return -1;

	printf("Installing ISR\n");

#if defined(MULTICORE)
	InstallTimerISR(1, vumeterTISR, TEN_MILLISECONDS_IN_TICKS);
	while(HARTMAILBOX[1] != 0x0) asm volatile("nop;");
#endif

	printf("Loading module\n");

	PlayMODFile("sd:test.mod");

	printf("Playback complete\n");

	return 0;
}
