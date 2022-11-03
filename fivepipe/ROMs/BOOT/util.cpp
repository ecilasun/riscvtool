
#include <cstdlib>

#include "core.h"
#include "fat32/ff.h"
#include "uart.h"
#include "elf.h"

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

	// Load all loadable sections
	for(unsigned short i=0; i<fheader->m_SHNum; ++i)
	{
		// Seek-load section headers as needed
		SElfSectionHeader32 sheader;
		f_lseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*i);
		f_read(fp, &sheader, sizeof(SElfSectionHeader32), &bytesread);

		// If this is a section worth loading...
		if (sheader.m_Flags & 0x00000007 && sheader.m_Size!=0)
		{
			// ...place it in memory
			uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
			f_lseek(fp, sheader.m_Offset);
			f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
		}
	}

	free(names);
}

void RunExecutable(const char *filename, const bool reportError)
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

		// Unmount volume so the next application can mount it again
		f_mount(nullptr, "sd:", 1);

        asm volatile(
            ".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
            "fence.i;"          // Invalidate I$
            "lw s0, %0;"        // Target branch address
            "jalr s0;"          // Branch to the entry point
            : "=m" (branchaddress) : : 
        );

		// TODO: Executables don't come back yet, deal with this then
		// f_mount(&Fs, "sd:", 1);
	}
	else
	{
		if (reportError)
		{
			UARTWrite("Executable '");
			UARTWrite(filename);
			UARTWrite("' not found.\n");
		}
	}
}
