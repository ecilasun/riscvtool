// Boot ROM

#include "rvcrt0.h"

#include "memtest/memtest.h"

#include <math.h>
#include <string.h>

#include "core.h"
#include "elf.h"
#include "fat32/ff.h"
#include "gpu.h"
#include "leds.h"
#include "uart.h"

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

void HandleUART()
{
	// *IO_UARTRXTX = 0x13; // XOFF

	// Echo back incoming bytes
	while (UARTHasData())
	{
		// Consume incoming character
		uint8_t incoming = *IO_UARTRX;
		// Echo back
		*IO_UARTTX = incoming;
	}

	// *IO_UARTRXTX = 0x11; // XON
}

void HandleTimer()
{
	static uint32_t nextstate = 0;
	LEDSetState(nextstate++);

	uint64_t now = E32ReadTime();
	uint64_t future = now + ONE_SECOND_IN_TICKS;
	E32SetTimeCompare(future);
}

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) interrupt_service_routine()
{
	//uint32_t a7;
	//asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined (this is the SYSCALL function code)

	//uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	//uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000)
	{
		// This is a hardware or timer interrupt
		if (code == IRQ_M_EXT)
		{
			// Machine External Interrupt (hardware)
			// Route based on hardware type
			if (value & 0x00000001) HandleUART();
			//if (value & 0x00000002) HandleKeyboard();
		}

		if (code == IRQ_M_TIMER)
		{
			// Machine Timer Interrupt (timer)
			HandleTimer();
		}
	}
	else
	{
		switch(code)
		{
			case CAUSE_MISALIGNED_FETCH:
			case CAUSE_FETCH_ACCESS:
			case CAUSE_ILLEGAL_INSTRUCTION:
			case CAUSE_BREAKPOINT:
			case CAUSE_MISALIGNED_LOAD:
			case CAUSE_LOAD_ACCESS:
			case CAUSE_MISALIGNED_STORE:
			case CAUSE_STORE_ACCESS:
			case CAUSE_USER_ECALL:
			case CAUSE_SUPERVISOR_ECALL:
			case CAUSE_HYPERVISOR_ECALL:
			case CAUSE_MACHINE_ECALL:
			case CAUSE_FETCH_PAGE_FAULT:
			case CAUSE_LOAD_PAGE_FAULT:
			case CAUSE_STORE_PAGE_FAULT:
			{
				// This is an exception
				//
				//HandleTrap(cause, a7, value, PC); // Illegal instruction etc
			}
			break;

			default: break;
		}
	}
}

void InstallISR()
{
	// Set machine trap vector
	swap_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + ONE_SECOND_IN_TICKS; // One seconds into the future
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Allow all machine interrupts to trigger
	swap_csr(mstatus, MSTATUS_MIE);
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
		UARTWrite("Loading boot executable...");
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

		// Unmount filesystem and reset to root directory before passing control
		UnmountDrive();

		UARTWrite("done. Launching\n");

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

int main()
{
	InstallISR();

	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2J");

	{
		UARTWrite("\nClearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF

		uint64_t startclock = E32ReadTime();
		UARTWrite("Start clock:");
		UARTWriteDecimal(startclock);

		for (uint32_t m=0x0A000000; m<0x0C000000; m+=4)
			*((volatile uint32_t*)m) = 0x00000000;

		uint64_t endclock = E32ReadTime();
		UARTWrite("\nEnd clock:");
		UARTWriteDecimal(endclock);

		uint32_t deltams = ClockToMs(endclock-startclock);
		UARTWrite("\nClearing 32Mbytes took ");
		UARTWriteDecimal((unsigned int)deltams);
		UARTWrite(" ms\n");

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

	// Load startup executable
	{
		MountDrive();
		RunExecutable(0, "sd:\\boot.elf", true);
	}

	while (1) {
		asm volatile ("wfi;");
	}

	return 0;
}
