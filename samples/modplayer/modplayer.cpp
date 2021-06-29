#include "core.h"
#include "apu.h"
#include "gpu.h"
#include "switches.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "micromod/micromod.h"

FATFS Fs;
uint32_t hardwareswitchstates, oldhardwareswitchstates;
volatile uint32_t *gpuSideSubmitCounter = (volatile uint32_t *)GraphicsFontStart-32;
uint32_t gpuSubmitCounter;
uint32_t vramPage = 0;
int selectedmodfile = 0;
int nummodfiles = 0;
char *modfiles[64];
int histogram[80];

uint32_t *onepixel = (uint32_t *)GraphicsRAMStart;

void SubmitPlaybackFrame()
{
	//if (*gpuSideSubmitCounter == gpuSubmitCounter)
	{
      // Next frame
      ++gpuSubmitCounter;

		// CLS
		GPUSetRegister(1, 0x10101010);
		GPUClearVRAMPage(1);

		for (int i=0;i<80;++i)
		{
			int py = 96+histogram[i];

			uint32_t sysramsource = uint32_t(onepixel);
			GPUSetRegister(4, sysramsource);

			uint32_t vramramtarget = (i*4+py*512)>>2;
			GPUSetRegister(5, vramramtarget);

			// Length of copy in DWORDs
			uint32_t dmacount = 1; // DWORD count = 1/4th the pixel count
			GPUKickDMA(4, 5, dmacount, false);
		}

		// Swap to other page to reveal previous render
		vramPage = (vramPage+1)%2;
		GPUSetRegister(2, vramPage);
		GPUSetVideoPage(2);

		// GPU will write value in G2 to address in G3 in the future
		GPUSetRegister(3, uint32_t(gpuSideSubmitCounter));
		GPUSetRegister(2, gpuSubmitCounter);
		GPUWriteToGraphicsMemory(2, 3);

		// Clear state, GPU will overwrite this when it reaches GPUSYSMEMOUT
		*gpuSideSubmitCounter = 0;
	}
}

void SubmitGPUFrame()
{
   // Do not submit more work if the GPU is not ready
   //if (*gpuSideSubmitCounter == gpuSubmitCounter)
   {
      // Next frame
      ++gpuSubmitCounter;

      // CLS
      GPUSetRegister(1, 0x16161616); // 4 dark gray pixels
      GPUClearVRAMPage(1);

      if (nummodfiles == 0)
      {
         PrintDMA(88, 92, "mod player", false);
      }
      else
      {
         int line = 0;
         for (int i=0;i<nummodfiles;++i)
         {
            PrintDMA(8, line, modfiles[i], (selectedmodfile == i) ? false : true);
            line += 8;
         }
      }

      // Stall GPU until vsync is reached
      GPUWaitForVsync();

      // Swap to other page to reveal previous render
      vramPage = (vramPage+1)%2;
      GPUSetRegister(2, vramPage);
      GPUSetVideoPage(2);

      // GPU will write value in G2 to address in G3 in the future
      GPUSetRegister(3, uint32_t(gpuSideSubmitCounter));
      GPUSetRegister(2, gpuSubmitCounter);
      GPUWriteToGraphicsMemory(2, 3);

      // Clear state, GPU will overwrite this when it reaches GPUSYSMEMOUT
      *gpuSideSubmitCounter = 0;
   }
}

#define SAMPLING_FREQ  44000  /* 44khz. */
#define REVERB_BUF_LEN 550    /* 6.25ms. */
#define OVERSAMPLE     2      /* 2x oversampling. */
#define NUM_CHANNELS   2      /* Stereo. */
#define BUFFER_SAMPLES 512  /* buffer size */

static short mix_buffer[ BUFFER_SAMPLES * NUM_CHANNELS * OVERSAMPLE ];
static short reverb_buffer[ REVERB_BUF_LEN ];
short *buffer = (short*)AudioRAMStart;//[ BUFFER_SAMPLES * NUM_CHANNELS ];
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

			// Generate 255-wide histogram
			//__builtin_memset( histogram, 0, 64*sizeof(uint32_t) );
			for (int i=0;i<80/*BUFFER_SAMPLES*/;++i)
				histogram[i] = buffer[i*2*4]/256;
				//histogram[src[i]%128]++;

			// TODO: If 'buffer' is placed in AudioRAM, the APU
			// could kick the synchronized copy so we can free the CPU

			// Audio FIFO will be drained at playback rate and
			// the CPU will stall to wait if the FIFO is full.
			// Therefore, no need to worry about synchronization.
			uint32_t *src = (uint32_t *)buffer;
			for (uint32_t i=0;i<BUFFER_SAMPLES;++i)
				*IO_AudioFIFO = src[i];

			// Down arrow to stop
			hardwareswitchstates = *IO_SwitchState;
			int stop = 0;
			switch(hardwareswitchstates ^ oldhardwareswitchstates)
			{
				case 0x20: stop = hardwareswitchstates&0x20 ? 0 : 1; break;
				default: break;
			};

			oldhardwareswitchstates = hardwareswitchstates;
			if (stop) playing = 0;

			if( samples_remaining <= 0 || result != 0 )
				playing = 0;

			SubmitPlaybackFrame();
		}
	}
	else
		printf("micromod_initialise failed\n");

	return result;
}

void ListDir(const char *path)
{
   nummodfiles = 0;
   DIR dir;
   FRESULT re = f_opendir(&dir, path);
   if (re == FR_OK)
   {
      FILINFO finf;
      do{
         re = f_readdir(&dir, &finf);
         if (re == FR_OK && dir.sect!=0)
         {
            // We're only interested in MOD files
            if (strstr(finf.fname,".mod"))
            {
               int olen = strlen(finf.fname) + 3;
               modfiles[nummodfiles] = (char*)malloc(olen+1);
               strcpy(modfiles[nummodfiles], "sd:");
               strcat(modfiles[nummodfiles], finf.fname);
               modfiles[nummodfiles][olen] = 0;
               ++nummodfiles;
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
}

void PlayMODFile(int file)
{
	signed char *module;
	long count, length;

	/* Read module file.*/
	length = read_module_length( modfiles[file] );
	if( length > 0 )
	{
		printf( "Module Data Length: %li bytes.\n", length );
		module = (signed char*)calloc( length, 1 );
		if( module != NULL )
		{
			count = read_file( modfiles[file], module, length );
			if( count < length )
				printf("Module file is truncated. %li bytes missing.\n", length - count );
			play_module( module );
			free( module );
		}
	}
}

int main( int argc, char **argv )
{
	// Read switch state at startup
	hardwareswitchstates = oldhardwareswitchstates = *IO_SwitchState;

	InitFont();
	// Initialize video page
	GPUSetRegister(2, vramPage);
	GPUSetVideoPage(2);
	*gpuSideSubmitCounter = 0;
	gpuSubmitCounter = 0;

	// Make text color white and text background color light blue
	uint32_t colorwhite = MAKERGBPALETTECOLOR(255, 255, 255);
	uint32_t colorblue = MAKERGBPALETTECOLOR(128, 128, 255);
	GPUSetRegister(1, colorwhite);
	GPUSetRegister(2, colorblue);
	GPUSetPaletteEntry(1, 255);
	GPUSetPaletteEntry(2, 0);

	*onepixel = 0x30303030;

	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt != FR_OK)
		return -1;

    ListDir("sd:");

	while(1)
	{
		hardwareswitchstates = *IO_SwitchState;

		int down = 0, up = 0, sel = 0;
		switch(hardwareswitchstates ^ oldhardwareswitchstates)
		{
			case 0x10: sel = hardwareswitchstates&0x10 ? 0 : 1; break;
			case 0x20: down = hardwareswitchstates&0x20 ? 0 : 1; break;
			case 0x40: up = hardwareswitchstates&0x40 ? 0 : 1; break;
			default: break;
		};

		oldhardwareswitchstates = hardwareswitchstates;

		if (up) selectedmodfile--;
		if (down) selectedmodfile++;
		if (selectedmodfile>=nummodfiles) selectedmodfile = 0;
		if (selectedmodfile<0) selectedmodfile = nummodfiles-1;
		if (sel) { PlayMODFile(selectedmodfile); }

		SubmitGPUFrame();
	}

	return 0;
}
