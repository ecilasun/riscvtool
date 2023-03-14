// Boot ROM

#include "fileops.h"

#include "core.h"
#include "elf.h"
#include "fat32/ff.h"
#include "uart.h"

#include <string.h>
#include <stdlib.h>

static char currentdir[512] = "sd:\\";
static int havedrive = 0;
FATFS *Fs = nullptr;

const char *FRtoString[]={
	"Succeeded\r\n",
	"A hard error occurred in the low level disk I/O layer\r\n",
	"Assertion failed\r\n",
	"The physical drive cannot work\r\n",
	"Could not find the file\r\n",
	"Could not find the path\r\n",
	"The path name format is invalid\r\n",
	"Access denied due to prohibited access or directory full\r\n",
	"Access denied due to prohibited access\r\n",
	"The file/directory object is invalid\r\n",
	"The physical drive is write protected\r\n",
	"The logical drive number is invalid\r\n",
	"The volume has no work area\r\n",
	"There is no valid FAT volume\r\n",
	"The f_mkfs() aborted due to any problem\r\n",
	"Could not get a grant to access the volume within defined period\r\n",
	"The operation is rejected according to the file sharing policy\r\n",
	"LFN working buffer could not be allocated\r\n",
	"Number of open files > FF_FS_LOCK\r\n",
	"Given parameter is invalid\r\n"
};

void MountDrive()
{
	if (havedrive)
		return;

	// Attempt to mount file system on micro-SD card
	Fs = (FATFS*)malloc(sizeof(FATFS));
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
	{
		havedrive = 0;
		UARTWrite(FRtoString[mountattempt]);
	}
	else
	{
		strcpy(currentdir, "sd:\\");
		havedrive = 1;
	}
}

void UnmountDrive()
{
	if (havedrive)
	{
		f_mount(nullptr, "sd:", 1);
		free(Fs);
		havedrive = 0;
	}
}

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
			UARTWrite("\033[0m\r\n");
		} while(1);

		f_closedir(&dir);
	}
	else
		UARTWrite(FRtoString[re]);
}

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &jumptarget)
{
	if (fheader->m_Magic != 0x464C457F)
	{
		UARTWrite(" failed: expecting 0x7F+'ELF'\r\n");
		return;
	}

	jumptarget = fheader->m_Entry;
	UINT bytesread = 0;

	// Read program headers
	for (uint32_t i=0; i<fheader->m_PHNum; ++i)
	{
		SElfProgramHeader32 pheader;
		f_lseek(fp, fheader->m_PHOff + fheader->m_PHEntSize*i);
		f_read(fp, &pheader, sizeof(SElfProgramHeader32), &bytesread);

		// Something here
		if (pheader.m_MemSz != 0)
		{
			uint8_t *memaddr = (uint8_t *)pheader.m_PAddr;
			// Initialize the memory range at target physical address
			// This can be larger than the loaded size
			memset(memaddr, 0x0, pheader.m_MemSz);
			// Load the binary
			f_lseek(fp, pheader.m_Offset);
			f_read(fp, memaddr, pheader.m_FileSz, &bytesread);
		}
	}
}

uint32_t LoadExecutable(const char *filename, const bool reportError)
{
	FIL fp;
	FRESULT fr = f_open(&fp, filename, FA_READ);
	if (fr == FR_OK)
	{
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

		asm volatile(
			".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			"fence.i;"          // Invalidate I$
		);

		return branchaddress;
	}
	else
	{
		if (reportError)
		{
			UARTWrite("Executable '");
			UARTWrite(filename);
			UARTWrite("' not found.\r\n");
		}
	}

	return 0;
}
