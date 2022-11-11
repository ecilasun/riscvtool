
#include <cstdlib>

#include "core.h"
#include "fat32/ff.h"
#include "uart.h"
#include "elf.h"
#include "sdcard.h"

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &jumptarget)
{
	if (fheader->m_Magic != 0x464C457F)
	{
		UARTWrite(" failed: expecting 0x7F+'ELF'");
		return;
	}

	jumptarget = fheader->m_Entry;

	// Read program header
	SElfProgramHeader32 pheader;
	f_lseek(fp, fheader->m_PHOff);
	UINT bytesread;
	f_read(fp, &pheader, sizeof(SElfProgramHeader32), &bytesread);

	// Read string table section header
	unsigned int stringtableindex = fheader->m_SHStrndx;
	SElfSectionHeader32 stringtablesection;
	f_lseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
	f_read(fp, &stringtablesection, sizeof(SElfSectionHeader32), &bytesread);

	// Allocate memory for and read string table
	char *names = (char *)malloc(stringtablesection.m_Size);
	f_lseek(fp, stringtablesection.m_Offset);
	f_read(fp, names, stringtablesection.m_Size, &bytesread);

	// Load all resident sections
	for(unsigned short i=0; i<fheader->m_SHNum; ++i)
	{
		// Seek-load section headers as needed
		SElfSectionHeader32 sheader;
		f_lseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*i);
		f_read(fp, &sheader, sizeof(SElfSectionHeader32), &bytesread);

		// If this is a section worth loading...
		if ((sheader.m_Flags & 0x00000007) && sheader.m_Size != 0)
		{
			UARTWrite("@0x");
			UARTWriteHex(sheader.m_Addr);
			UARTWrite("[0x");
			UARTWriteHex(sheader.m_Size);
			UARTWrite("]\n");

			// ...place it in memory
			uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
			f_lseek(fp, sheader.m_Offset);
			f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
		}
	}

	free(names);
}

void LoadExecutable(const char *filename, const bool run)
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

		if (run)
		{
			// Unmount volume so the next application can mount it again
			f_mount(nullptr, "sd:", 1);

			UARTWrite("Branching to 0x");
			UARTWriteHex(branchaddress);
			UARTWrite("\n");

			asm volatile(
				".word 0xFC000073;" // Invalidate & write Back D$ (CFLUSH.D.L1)
				"fence.i;"          // Invalidate I$ & wait for all D$ operations to be visible (FENCE.I)
				"lw s0, %0;"        // Target branch address
				"jalr s0;"          // Branch to the entry point
				: "=m" (branchaddress) : : 
			);

			// TODO: Executables can not return here, fix it!
			// f_mount(&Fs, "sd:", 1);
		}
		else
		{
			UARTWrite("Loaded. Entry point @0x");
			UARTWriteHex(branchaddress);
			UARTWrite("\n");
			uint32_t *ptr = (uint32_t*)branchaddress;
			for (uint32_t a = 0; a<4096; ++a)
			{
				UARTWriteHex(ptr[a]);
				UARTWrite(" ");
			}
		}
	}
	else
	{
		UARTWrite("Executable '");
		UARTWrite(filename);
		UARTWrite("' not found.\n");
	}
}
