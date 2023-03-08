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

			//char *isexe = strstr(finf.fname, ".elf");
			//int isdir = finf.fattrib&AM_DIR;
			//if (isdir)
			//	UARTWrite("\033[32m"); // Green
			//if (isexe!=nullptr)
			//	UARTWrite("\033[33m"); // Yellow
			UARTWrite(finf.fname);
			//if (isdir)
			//	UARTWrite(" <dir>");
			/*else
			{
				UARTWrite(" ");
				UARTWriteDecimal((int32_t)finf.fsize);
				UARTWrite("b");
			}*/
			//UARTWrite("\033[0m");
			UARTWrite("\n");

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
		UARTWrite(" failed: expecting 0x7F+'ELF'");
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

void RunExecutable(const int hartID, const char *filename, const bool reportError)
{
	FIL fp;
	FRESULT fr = f_open(&fp, filename, FA_READ);
	if (fr == FR_OK)
	{
		//UARTWrite("Loading boot executable...");
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

		// Unmount filesystem and reset to root directory before passing control
		UnmountDrive();

		//UARTWrite("done. Launching\n");

		// Run the executable
		asm volatile(
			".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			"fence.i;"          // Invalidate I$
			"lw s0, %0;"        // Target branch address
			"jalr s0;"          // Branch to the entry point
			: "=m" (branchaddress) : : 
		);

		// Re-mount filesystem before re-gaining control, if execution falls back here
		MountDrive();
	}
	/*else
	{
		if (reportError)
		{
			UARTWrite("Executable '");
			UARTWrite(filename);
			UARTWrite("' not found.\n");
		}
	}*/
}