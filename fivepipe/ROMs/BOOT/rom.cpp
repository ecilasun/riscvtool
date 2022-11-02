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
#include "ps2.h"

#include "isr.h"
#include "util.h"

#include "memtest/memtest.h"

static char currentdir[512] = "sd:";
static char commandline[512] = "";
static char filename[128] = "";
static int cmdlen = 0;

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

int ProcessKeyEvents()
{
	static int uppercase = 0;
	int parseit = 0;

	// Any pending keyboard events to handle?
	uint32_t val = 0;
	// Consume one entry per execution
	swap_csr(mie, MIP_MSIP | MIP_MTIP);
	int R = RingBufferRead(&val, 4);
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);
	if (R)
	{
		uint32_t key = val&0xFF;
		// Toggle caps
		if (key == 0x58)
			uppercase = !uppercase;

		// Key up
		if (val&0x00000100)
		{
			if (key == 0x12 || key == 0x59) // Right/left shift up
				uppercase = 0;
		}
		else // Key down
		{
			if (key == 0x12 || key == 0x59) // Right/left shift down
				uppercase = 1;
			else
			{
				if (key == 0x5A) // Enter
					parseit = 1;
				else if (key == 0x66) // Backspace
				{
					cmdlen--;
					if (cmdlen < 0) cmdlen = 0;
					commandline[cmdlen] = 0;
				}
				else
				{
					commandline[cmdlen++] = ScanToASCII(val, uppercase);
					if (cmdlen>=511)
						cmdlen = 511;
					commandline[cmdlen] = 0;
				}
				UARTPutChar(ScanToASCII(val, uppercase));
			}
		}
	}

	return parseit;
}

void ParseCommands()
{
	// Grab first token, if any
	const char *command = strtok(commandline, " ");

	if (!strcmp(command, "help")) // Help text
	{
		UARTWrite("\033[34m\033[47m\033[7mdir\033[0m: show list of files in working directory\n");
		UARTWrite("\033[34m\033[47m\033[7mcwd\033[0m: change working directory\n");
		UARTWrite("\033[34m\033[47m\033[7mpwd\033[0m: show working directory\n");
	}
	else if (!strcmp(command, "cwd"))
	{
		// Use first parameter to set current directory
		char *param = strtok(nullptr, " ");
		if (param != nullptr)
			strcpy(currentdir, param);
		else
			strcpy(currentdir, "sd:");
	}
	else if (!strcmp(command, "pwd"))
	{
		UARTWrite(currentdir);
		UARTWrite("\n");
	}
	else if (!strcmp(command, "dir")) // List directory
	{
		UARTWrite("\n");
		UARTWrite(currentdir);
		UARTWrite("\n");
		ListFiles(currentdir);
	}
	else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
	{
		// Build a file name from the input string
		strcpy(filename, currentdir);
		strcat(filename, command);
		strcat(filename, ".elf");
		RunExecutable(filename, true);
	}

	cmdlen = 0;
	commandline[0] = 0;
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
	LEDSetState(0);
	InitRingBuffer();

	UARTWrite("┌─────────────────────────┐\n");
	UARTWrite("│ E32OS v0.0              │\n");
	UARTWrite("│ (c)2022 Engin Cilasun   │\n");
	UARTWrite("└─────────────────────────┘\n\n");

	// File system test
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		UARTWrite(FRtoString[mountattempt]);

	// DDR3 access test
	{
		UARTWrite("\nTesting DDR3 on AXI4 bus\n");

		UARTWrite("Clearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF
		uint64_t startclock = E32ReadTime();
		for (uint32_t m=0x0A000000; m<0x0C000000; m+=4)
			*((volatile uint32_t*)m) = 0x00000000;

		uint64_t endclock = E32ReadTime();
		uint32_t deltams = ClockToMs(endclock-startclock);
		UARTWrite("Clearing 32Mbytes took ");
		UARTWriteDecimal((unsigned int)deltams);
		UARTWrite(" ms\n");

		int rate = (1024*32*1024) / deltams;
		UARTWrite("Zero-write rate is ");
		UARTWriteDecimal(rate);
		UARTWrite(" Kbytes/sec\n");

		UARTWrite("\n-------------MemTest--------------\n");
		UARTWrite("Copyright (c) 2000 by Michael Barr\n");
		UARTWrite("----------------------------------\n");

		UARTWrite("Data bus test (0x0A000000-0x0A03FFFF)...");
		int failed = 0;
		for (uint32_t i=0x0A000000; i<0x0A03FFFF; i+=4)
		{
			failed += memTestDataBus((volatile datum*)i);
		}
		UARTWrite(failed == 0 ? "passed (" : "failed (");
		UARTWriteDecimal(failed);
		UARTWrite(" failures)\n");

		UARTWrite("Address bus test (0x0B000000-0x0B03FFFF)...");
		int errortype = 0;
		datum* res = memTestAddressBus((volatile datum*)0x0B000000, 262144, &errortype);
		UARTWrite(res == NULL ? "passed\n" : "failed\n");
		if (res != NULL)
		{
			if (errortype == 0)
				UARTWrite("Reason: Address bit stuck high at 0x");
			if (errortype == 1)
				UARTWrite("Reason: Address bit stuck low at 0x");
			if (errortype == 2)
				UARTWrite("Reason: Address bit shorted at 0x");
			UARTWriteHex((unsigned int)res);
			UARTWrite("\n");
		}

		UARTWrite("Memory device test (0x0C000000-0x0C03FFFF)...");
		datum* res2 = memTestDevice((volatile datum *)0x0C000000, 262144);
		UARTWrite(res2 == NULL ? "passed\n" : "failed\n");
		if (res2 != NULL)
		{
			UARTWrite("Reason: incorrect value read at 0x");
			UARTWriteHex((unsigned int)res2);
			UARTWrite("\n");
		}

		if ((failed != 0) | (res != NULL) | (res2 != NULL))
			UARTWrite("Memory device does not appear to be working correctly.\n");
	}

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

	while(1)
	{
		// Main loop will only wake up at hardware interrupt requests
		asm volatile("wfi;");

		// Handle input
		if (ProcessKeyEvents())
			ParseCommands();
	}

	return 0;
}

/*
	uint32_t prevVoltage = 0xC004CAFE;
	uint32_t voltage = 0x00000000;
	uint32_t x = 0;

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
*/