// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "buttons.h"
#include "uart.h"
#include "elf.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "ps2.h"
#include "ringbuffer.h"

#define ROMREVISION "v0.174"

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

static char currentdir[512] = "sd:\\";
static char commandline[512] = "";
static char filename[128] = "";
static int cmdlen = 0;
static int havedrive = 0;

// Main file system object
FATFS *Fs;

// Keyboard event ring buffer (1024 bytes)
uint8_t *keyboardringbuffer = (uint8_t*)0x80000200; // 512 bytes into mailbox memory
// Keyboard map is at top of S-RAM (512 bytes)
uint16_t keymap[256];
// Previous key map to be able to track deltas (512 bytes)
uint16_t keymapprev[256];

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

void EchoUART()
{
	// *IO_UARTRXTX = 0x13; // XOFF
	//UARTFlush();

	// Echo back incoming bytes
	while ((*IO_UARTStatus)&0x00000001)
	{
		// Consume incoming character
		uint8_t incoming = *IO_UARTRX;
		// TODO: Handle debugger / file server etc
		// Echo back
		*IO_UARTTX = incoming;
	}
	UARTFlush();

	// *IO_UARTRXTX = 0x11; // XON
	//UARTFlush();
}

void HandleButtons()
{
	while (*BUTTONCHANGEAVAIL)
	{
		uint32_t change = *BUTTONCHANGE;

		UARTWrite("Button state change: ");
		UARTWriteHex(change);
		UARTWrite("\n");
	}
}

void HandleTimer()
{
	// Show boot message
	UARTWrite("\033[H\033[0m\033[2J");
	UARTWrite("┌─────────────────────────┐\n");
	UARTWrite("│          ▒▒▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
	UARTWrite("│ ████████   ▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
	UARTWrite("│ █████████  ▒▒▒▒▒▒▒▒▒▒▒▒ │\n");
	UARTWrite("│ ████████   ▒▒▒▒▒▒▒▒▒▒▒  │\n");
	UARTWrite("│ █        ▒▒▒▒▒▒▒▒▒▒▒    │\n");
	UARTWrite("│ ██   ▒▒▒▒▒▒▒▒▒▒▒▒▒   ██ │\n");
	UARTWrite("│ ████   ▒▒▒▒▒▒▒▒▒   ████ │\n");
	UARTWrite("│ ██████   ▒▒▒▒▒   ██████ │\n");
	UARTWrite("│ ████████   ▒   ████████ │\n");
	UARTWrite("│ ██████████   ██████████ │\n");
	UARTWrite("│                         │\n");
	UARTWrite("│ E32OS " ROMREVISION "            │\n");
	UARTWrite("│ (c)2022 Engin Cilasun   │\n");
	UARTWrite("└─────────────────────────┘\n\n");

	// Stop further timer interrupts by setting timecmp to furthest value available.
	swap_csr(0x801, 0xFFFFFFFF);
	swap_csr(0x800, 0xFFFFFFFF);
}

void HandleKeyboard()
{
	// Consume all key state changes from FIFO and update the key map
	while (*PS2KEYBOARDDATAAVAIL)
		PS2ScanKeyboard(keymap);

	// If there's a difference between the previous keymap and current one, generate events for each change
	for (uint32_t i=0; i<256; ++i)
	{
		// Skip keyboard OK signal (occurs when first plugged in)
		if (i==0xAA)
			continue;

		// Generate key up/down events
		uint32_t prevval = (uint32_t)keymapprev[i];
		uint32_t val = (uint32_t)keymap[i];
		if (prevval^val) // Mismatch, this considered an event
		{
			// Store new state in previous state buffer since we're done reading it
			keymapprev[i] = val;
			// Wait for adequate space in ring buffer to write
			// NOTE: We'll simply hang if ringbuffer is full because nothing else
			// is running on this core during ISR. So attempt once, and bail out
			// if we can't write...
			//while(RingBufferWrite(keyboardringbuffer, &val, 4) == 0) { }
			RingBufferWrite(keyboardringbuffer, &val, 4);
		}
	}
}

void HandleTrap(const uint32_t cause, const uint32_t a7, const uint32_t value, const uint32_t PC)
{
	// NOTE: One use of illegal instruction exception would be to do software emulation of the instruction in 'value'.
	switch (cause)
	{
		case CAUSE_BREAKPOINT:
		{
			// TODO: Debugger related
		}
		break;

		//case CAUSE_USER_ECALL:
		//case CAUSE_SUPERVISOR_ECALL:
		//case CAUSE_HYPERVISOR_ECALL:
		case CAUSE_MACHINE_ECALL:
		{

			// NOTE: See \usr\include\asm-generic\unistd.h for a full list

			// A7
			// 64  sys_write  -> print (A0==1->stdout, A1->string, A2->length)
			// 96  sys_exit   -> terminate (A0==return code)
			// 116 sys_syslog -> 
			// 117 sys_ptrace -> 

			// TODO: implement system calls

			UARTWrite("\033[31m\033[40m");

			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Unimplemented machine ECALL. Program will resume  │\n");
			UARTWrite("│ execution, though it might crash.                 │\n");
			UARTWrite("│ #");
			UARTWriteHex((uint32_t)a7); // A7 containts Syscall ID
			UARTWrite(" @");
			UARTWriteHex((uint32_t)PC); // PC
			UARTWrite("                               │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");
		}
		break;

		//case CAUSE_MISALIGNED_FETCH:
		//case CAUSE_FETCH_ACCESS:
		//case CAUSE_ILLEGAL_INSTRUCTION:
		//case CAUSE_MISALIGNED_LOAD:
		//case CAUSE_LOAD_ACCESS:
		//case CAUSE_MISALIGNED_STORE:
		//case CAUSE_STORE_ACCESS:
		//case CAUSE_FETCH_PAGE_FAULT:
		//case CAUSE_LOAD_PAGE_FAULT:
		//case CAUSE_STORE_PAGE_FAULT:
		default:
		{
			UARTWrite("\033[0m\n\n"); // Clear attributes, step down a couple lines

			// reverse on: \033[7m
			// blink on: \033[5m
			// Set foreground color to red and bg color to black
			UARTWrite("\033[31m\033[40m");

			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Software Failure. Press reset button to continue. │\n");
			UARTWrite("│   Guru Meditation #");
			UARTWriteHex((uint32_t)cause); // Cause
			UARTWrite(".");
			UARTWriteHex((uint32_t)value); // Failed instruction
			UARTWrite(" @");
			UARTWriteHex((uint32_t)PC); // PC
			UARTWrite("    │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");

			// Put core to endless sleep
			while(1) {
				asm volatile("wfi;");
			}
		}
		break; // Doesn't make sense but to make compiler happy...
	}
}

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) interrupt_service_routine()
{
	uint32_t a7;
	asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined.

	uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000) // Interrupt
	{
		if (code == 0xB) // Machine External Interrupt (hardware)
		{
			// Route based on hardware type
			if (value & 0x00000001) EchoUART();
			if (value & 0x00000002) HandleButtons();
			if (value & 0x00000004) HandleKeyboard();
			//if (value & 0x00000010) HandleHARTIRQ(hartid); // HART#0 doesn't use this
		}

		if (code == 0x7) // Machine Timer Interrupt (timer)
		{
			HandleTimer();
		}
	}
	else // Machine Software Exception (trap)
	{
		HandleTrap(cause, a7, value, PC);
	}
}

void InstallISR()
{
	// Set machine trap vector
	swap_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + ONE_SECOND_IN_TICKS; // Two seconds into the future (based on 10MHz wall clock)
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

void RunExecutable(const char *filename, const bool reportError)
{
	FIL fp;
	FRESULT fr = f_open(&fp, filename, FA_READ);
	if (fr == FR_OK)
	{
		UARTWrite("Loading...\n");
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

		UARTWrite("Starting...\n");

		// Unmount filesystem and reset to root directory before passing control
		UnmountDrive();

		// Run the executable
		asm volatile(
			".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			"fence.i;"          // Invalidate I$
			"lw s0, %0;"        // Target branch address
			"jalr s0;"          // Branch to the entry point
			: "=m" (branchaddress) : : 
		);

		// Re-mount filesystem before re-gaining control
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

void ParseCommands()
{
	// Grab first token, if any
	const char *command = strtok(commandline, " ");

	if (!strcmp(command, "help")) // Help text
	{
		UARTWrite("dir: list files\n");
		UARTWrite("cd: change directory\n");
		UARTWrite("pd: show directory\n");
	}
	else if (!strcmp(command, "cwd"))
	{
		// Use first parameter to set current directory
		char *param = strtok(nullptr, " ");
		if (param != nullptr)
		{
			strcpy(currentdir, param);
			strcat(currentdir, "\\");
		}
		else
			strcpy(currentdir, "sd:\\");
	}
	else if (!strcmp(command, "pwd"))
	{
		UARTWrite(currentdir);
		UARTWrite("\n");
	}
	else if (!strcmp(command, "dir")) // List directory
	{
		UARTWrite("\n");
		MountDrive();
		if (havedrive)
		{
			UARTWrite(currentdir);
			UARTWrite("\n");
			ListFiles(currentdir);
		}
	}
	else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
	{
		MountDrive();
		if (havedrive)
		{
			// Build a file name from the input string
			strcpy(filename, currentdir);
			strcat(filename, command);
			strcat(filename, ".elf");
			RunExecutable(filename, true);
		}
	}

	cmdlen = 0;
	commandline[0]=0;
}

int ProcessKeyEvents()
{
	static int uppercase = 0;
	int parseit = 0;

	// Any pending keyboard events to handle?
	uint32_t val = 0;
	// Consume one entry per execution
	swap_csr(mie, MIP_MSIP | MIP_MTIP);
	int R = RingBufferRead(keyboardringbuffer, &val, 4);
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

// Worker CPU entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while (1)
	{
		asm volatile("wfi;");
	}
}

int main()
{
	LEDSetState(0x0);

	// Install interrupt service routines
	InstallISR();

	// Main loop, sleeps most of the time until an interrupt occurs
	while(1)
	{
		// Interrupt handler will do all the real work.
		// Therefore we can put the core to sleep until an interrupt occurs.
		asm volatile("wfi;");

		// Wake up to process keyboard events.
		if (ProcessKeyEvents())
			ParseCommands();
	}

	return 0;
}
