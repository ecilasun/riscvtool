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
#include "sdcard.h"

#include "isr.h"
#include "util.h"

static char currentdir[512] = "sd:";
static char execname[512] = "unknown";
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
	int R = PS2RingBufferRead(&val, 4);
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
					commandline[cmdlen++] = PS2ScanToASCII(val, uppercase);
					if (cmdlen>=511)
						cmdlen = 511;
					commandline[cmdlen] = 0;
				}
				UARTPutChar(PS2ScanToASCII(val, uppercase));
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
		UARTWrite("\033[34m\033[47m\033[7minit\033[0m: initialize DDR3 memory\n");
		UARTWrite("\033[34m\033[47m\033[7msdtest\033[0m: turn sdcard off and on rapidly for test\n");
		UARTWrite("\033[34m\033[47m\033[7mtemp\033[0m: show curent temperature\n");
		UARTWrite("\033[34m\033[47m\033[7mcls\033[0m: clear visible portion of terminal\n");
		UARTWrite("\033[34m\033[47m\033[7mload\033[0m: load given ELF without starting it\n");
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
	else if (!strcmp(command, "temp"))
	{
		uint32_t tempsample = *XADCTEMP;
		int temperature_centigrates = (tempsample*504) / 4096 - 273;
		UARTWrite("Approximate temperature: ");
		UARTWriteDecimal(temperature_centigrates);
		UARTWrite(" C\n");
	}
	else if (!strcmp(command, "init"))
	{
		UARTWrite("Initializing DDR3: ");
		uint32_t *ddr3mem = (uint32_t*)0;
		const uint32_t numwords = 0x1FFFF000 / 4;
		for (uint32_t i=0;i<numwords;++i)
		{
			if ((i%0x40000) == 0)
				UARTWrite("█");
			ddr3mem[i] = 0xCDCDCDCD;
		}
		UARTWrite(" Done\n");
	}
	else if (!strcmp(command, "sdtest"))
	{
		UARTWrite("Disconnecting and powering down SDCard...\n");
		f_mount(nullptr, "sd:", 1);
		SDCardControl(0x0);

		UARTWrite("Powering up and connecting SDCard...\n");
		SDCardControl(0x3);
		FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
		if (mountattempt!=FR_OK)
			UARTWrite(FRtoString[mountattempt]);
		else
			UARTWrite("Success\n");
	}
	else if(!strcmp(command, "cls"))
	{
		UARTWrite("\033[H\033[0m\033[2J");
	}
	else if (!strcmp(command, "dir")) // List directory
	{
		UARTWrite("\n");
		UARTWrite(currentdir);
		UARTWrite("\n");
		ListFiles(currentdir);
	}
	else if (!strcmp(command, "load"))
	{
		// Use first parameter to set current directory
		char *param = strtok(nullptr, " ");
		if (param != nullptr)
			strcpy(execname, param);
		else
			strcpy(execname, "unknown");
		LoadExecutable(execname, false);
	}
	else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
	{
		// Build a file name from the input string
		strcpy(filename, currentdir);
		strcat(filename, command);
		strcat(filename, ".elf");
		LoadExecutable(filename, true);
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
	PS2InitRingBuffer();

	// File system test
	UARTWrite("Mounting filesystem\n");
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		UARTWrite(FRtoString[mountattempt]);

	// Only on first core
	UARTWrite("Installing ISR\n");
	uint32_t hartid = read_csr(mhartid);
	if (hartid == 0)
	{
		// Ready to handle hardware & software interrupts
		InstallMainISR();
	}

	UARTWrite("Ready\n");
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
