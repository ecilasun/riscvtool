// Bootloader

// (c)2022 Engin Cilasun

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <math.h>

#include "rvcrt0.h"

#include "fat32/ff.h"

#include "core.h"
#include "leds.h"
#include "uart.h"
#include "xadc.h"
#include "gpu.h"
#include "isr.h"

const char *FRtoString[]={
	"Succeeded\n",
	"A hard error occurred in the low level disk I/O layer\n",
	"Assertion failed\n",
	"The physical drive cannot work\n",
	"Could not find the file\n",
	"Could not find the path\n",
	"The path name format is invalid\n",
	"Access denied due to prohibited access or directory full\n",
	"Access denied due to prohibited access\n",
	"The file/directory object is invalid\n",
	"The physical drive is write protected\n",
	"The logical drive number is invalid\n",
	"The volume has no work area\n",
	"There is no valid FAT volume\n",
	"The f_mkfs() aborted due to any problem\n",
	"Could not get a grant to access the volume within defined period\n",
	"The operation is rejected according to the file sharing policy\n",
	"LFN working buffer could not be allocated\n",
	"Number of open files > FF_FS_LOCK\n",
	"Given parameter is invalid\n"
};

// Main file system object
FATFS Fs;

void ListFiles(const char *path)
{
	DIR dir;
	FRESULT re = f_opendir(&dir, path);
	if (re == FR_OK)
	{
		FILINFO finf;
		do{
			re = f_readdir(&dir, &finf);
			if (re != FR_OK || finf.fname[0] == 0) // Done scanning dir, or error encountered
				break;

			char *isexe = strstr(finf.fname, ".elf");
			int isdir = finf.fattrib&AM_DIR;
			if (isdir)
				UARTWrite("\033[32m"); // Green
			if (isexe!=nullptr)
				UARTWrite("\033[33m"); // Yellow
			UARTWrite(finf.fname);
			if (isdir)
				UARTWrite(" <dir>");
			else
			{
				UARTWrite(" ");
				UARTWriteDecimal((int32_t)finf.fsize);
				UARTWrite("b");
			}
			UARTWrite("\033[0m\n");

		} while(1);

		f_closedir(&dir);
	}
	else
		UARTWrite(FRtoString[re]);
}

// HART[1..N-1] entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}
}

// HART[0] entry point
int main()
{
 	// Reset debug LEDs
	LEDSetState(0x0);

	// File system test
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		UARTWrite(FRtoString[mountattempt]);
	else
		ListFiles("sd:");

	// Set low bit RGB color palette
	/*uint8_t target = 0;
	for (int b=0;b<4;++b)
	for (int g=0;g<8;++g)
		for (int r=0;r<8;++r)
		GPUSetPal(target++, MAKECOLORRGB24(r*32, g*32, b*64));*/

	// Set scan-out address
	// One can allocate some memory aligned to 64 bytes and pass
	// the pointer to this function to set that memory to be the display scan-out
	//GPUSetVPage((uint32_t)VRAM); // This is already the default

	// Only on first core
	uint32_t hartid = read_csr(mhartid);
	if (hartid == 0)
	{
		// Ready to handle hardware & software interrupts
		InstallMainISR();
	}

	uint32_t prevVoltage = 0xC004CAFE;
	uint32_t voltage = 0x00000000;
	uint32_t x = 0;

	while(1)
	{
		// Main loop will only wake up at hardware interrupt requests
		//asm volatile("wfi;");

		// While we're awake, also run some voltage measurements
		voltage = (voltage + *XADCPORT)>>1;
		if (prevVoltage != voltage)
		{
			// Clear the line ahead
			for (int k=0;k<240;++k)
				VRAMBYTES[(x+1)%320 + k*320] = 0xFF;
			// Show voltage value
			uint32_t y = (voltage/273)%240;
			VRAMBYTES[x + y*320] = 0x0F;
			if (x==320)
			{
				// Flush data cache so we can see what's happening (entire cache, switch to use one with only one cache page)
				asm volatile( ".word 0xFC000073;");
			}
			// Next column
			x = (x+1)%320;
			prevVoltage = voltage;
		}

		// Flip between vram page 0 and page 1
		//GPUSetVPage((uint32_t)VRAM + (scrollingleds%2)*320*240);

	}

	return 0;
}
