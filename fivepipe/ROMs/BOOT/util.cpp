
#include <cstdlib>
#include <string.h>

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
	UINT bytesread = 0;

	// Read program headers
	for (uint32_t i=0; i<fheader->m_PHNum; ++i)
	{
		SElfProgramHeader32 pheader;
		f_lseek(fp, fheader->m_PHOff + fheader->m_PHEntSize*i);
		f_read(fp, &pheader, sizeof(SElfProgramHeader32), &bytesread);

		UARTWrite("o: 0x");
		UARTWriteHex(pheader.m_Offset);
		UARTWrite(" pa: 0x");
		UARTWriteHex(pheader.m_PAddr);
		UARTWrite(" fs:0x");
		UARTWriteHex(pheader.m_FileSz);
		UARTWrite(" ms:0x");
		UARTWriteHex(pheader.m_MemSz);
		UARTWrite(" f:0x");
		UARTWriteHex(pheader.m_Flags);
		UARTWrite(" t:0x");
		UARTWriteHex(pheader.m_Type);
		UARTWrite("\n");

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
