// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "uart.h"
#include "dma.h"
#include "elf.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "ps2.h"
#include "ringbuffer.h"

// Make sure to keep this 6 characters long
#define ROMREVISION "v0.182"

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
static uint32_t numdetectedharts = 0;
static int defaultHardID = 0;

// Space for per-hart data lives after stack space
static uint32_t *OSScratchMem = (uint32_t*)0x1FFF0000;

// Main file system object
FATFS *Fs;

// Keyboard event ring buffer (1024 bytes)
uint8_t *keyboardringbuffer = (uint8_t*)RINGBUFFER_ADDRESS; // 512 bytes into mailbox memory
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
	// Report live HART count
	for (uint32_t i=1; i<NUM_HARTS; ++i)
		if (HARTMAILBOX[i] == 0xE32EBABE) // This is an E32E core, consider alive
			numdetectedharts++;

	// Show boot message
	UARTWrite("┌─────────────────────────┐\n");
	UARTWrite("│ E32OS " ROMREVISION "            │\n");
	UARTWrite("│ (c)2022 Engin Cilasun   │\n");
	UARTWrite("│                         │\n");
	UARTWrite("│ HARTS found: ");// Expecting single digit HART count here
	UARTWriteDecimal(numdetectedharts);
	UARTWrite("          │\n");
	UARTWrite("└─────────────────────────┘\n\n");

	// Stop further timer interrupts by setting timecmp to furthest value available.
	swap_csr(0x801, 0xFFFFFFFF);
	swap_csr(0x800, 0xFFFFFFFF);
}

void HandleKeyboard()
{
	// Consume all key state changes from FIFO and update the key map
	PS2ScanKeyboard(keymap);

	// If there's a difference between the previous keymap and current one, generate events for each change
	for (uint32_t i=0; i<256; ++i)
	{
		// Skip keyboard OK (when plugged in)
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
			// TODO: We will get stuck here every time as long as the debugger does not replace this instruction
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
			if (value & 0x00000001) HandleUART();
			if (value & 0x00000002) HandleKeyboard();
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

void HandleHARTIRQ(uint32_t hartid)
{
	// Answer different service IDs when woken up
	// by another HART via HARTIRQ + request code in HARTMAILBOX

	uint32_t REQ = HARTMAILBOX[hartid];
	switch (REQ)
	{
		// REQ#0 : Alive check
		case REQ_CheckAlive:
		{
			HARTMAILBOX[hartid] = 0xE32EBABE; // Signal alive into our mail slot
		}
		break;

		// REQ#1 : Install user timer interrupt handler (TISR) and set its interval
		case REQ_InstallTISR:
		{
			// TISR address, parameter #0
			uint32_t TISR = HARTMAILBOX[hartid*HARTPARAMCOUNT+0+NUM_HARTS];

			// TISR interval, parameter #1
			uint32_t TISR_Interval = HARTMAILBOX[hartid*HARTPARAMCOUNT+1+NUM_HARTS];

			// Store for later retreival
			OSScratchMem[hartid*NUMHARTWORDS+0] = TISR;
			OSScratchMem[hartid*NUMHARTWORDS+1] = TISR_Interval;

			// Will start after interval
			uint64_t now = E32ReadTime();
			uint64_t future = now + TISR_Interval;
			E32SetTimeCompare(future);

			HARTMAILBOX[hartid] = 0x00000000; // Done
		}
		break;

		// REQ#2 : Start executable on main thread of this HART
		case REQ_StartExecutable:
		{
			// Let the main thread know that we're supposed to start an executable at the given address
			//uint32_t progaddress = HARTMAILBOX[hartid*HARTPARAMCOUNT+0+NUM_HARTS];
			//CDISCARD_D_L1;
			//OSScratchMem[hartid*NUMHARTWORDS+2] = progaddress;
			//OSScratchMem[hartid*NUMHARTWORDS+3] = 0x1;
		}
		break;

		// NOOP
		default:
		{
			HARTMAILBOX[hartid] = 0x00000000; // Done
		}
		break;
	}

	// Request handled, clear interrupt
	HARTIRQ[hartid] = 0;
}

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) worker_interrupt_service_routine()
{
	uint32_t a7;
	asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined.

	uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	//uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;
	uint32_t hartid = read_csr(mhartid);

	if (cause & 0x80000000) // Interrupt
	{
		if (code == 0xB) // Machine External Interrupt (hardware)
		{
			// HARTs 1..7 don't use these yet (see service request handler below to add support)
			//if (value & 0x00000001) HandleUART();
			//if (value & 0x00000002) HandleKeyboard();

			// Service request handler
			if (value & 0x00000010) HandleHARTIRQ(hartid);
		}

		if (code == 0x7) // Machine Timer Interrupt (timer)
		{
			uint32_t keepAlive = 0;
			// Chain into the TISR if there is one installed
			uint32_t TISR = OSScratchMem[hartid*NUMHARTWORDS+0];

			// If there's an installed routine, call it
			if (TISR)
			{
				((t_timerISR)TISR)(hartid);
				keepAlive = HARTMAILBOX[hartid*HARTPARAMCOUNT+0+NUM_HARTS];
			}

			if (keepAlive) // TISR wishes to stay alive for next time
			{
				// Re-set the timer interval to wake up again
				uint32_t TISR_Interval = OSScratchMem[hartid*NUMHARTWORDS+1];
				uint64_t now = E32ReadTime();
				uint64_t future = now + TISR_Interval;
				E32SetTimeCompare(future);
			}
			else // TISR wishes to be terminated
			{
				// Uninstall the routine
				OSScratchMem[hartid*NUMHARTWORDS+0] = 0x0;
				OSScratchMem[hartid*NUMHARTWORDS+1] = 0x0;

				// Set timecmp to a distant future
				swap_csr(0x801, 0xFFFFFFFF);
				swap_csr(0x800, 0xFFFFFFFF);
			}
		}
	}
	else // Machine Software Exception (trap)
	{
		//HandleTrap(cause, a7, value, PC);
	}
}

void InstallWorkerISR()
{
	// Set machine trap vector
	swap_csr(mtvec, worker_interrupt_service_routine);

	// Set up timer interrupt for this HART
	//uint64_t now = E32ReadTime();
	//uint64_t future = now + 512; // Set to happen very soon, around similar points in time for all HARTs except #0
	//E32SetTimeCompare(future);

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

void ClearTerminal()
{
	UARTWrite("\033[H\033[0m\033[2J");
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
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

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
	//else if (!strcmp(command, "hart")) // Set hart to run the loaded executable
	//{
	//	char *param = strtok(nullptr, " ");
	//	if (param != nullptr)
	//		defaultHardID = atoi(param);
	//	else
	//		defaultHardID = 0;
	//}
	else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
	{
		MountDrive();
		if (havedrive)
		{
			// Build a file name from the input string
			strcpy(filename, currentdir);
			strcat(filename, command);
			strcat(filename, ".elf");
			RunExecutable(defaultHardID, filename, true);
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
	InstallWorkerISR(); // In a very short while, this will trigger a timer interrupt to test 'alive'

	//uint32_t hartid = read_csr(mhartid);

	while (1)
	{
		/*if ((hartid==1) && (HARTMAILBOX[1] != 0))
		{
			asm volatile(
				".word 0xFC200073;" // CDISCARD.D.L1
			);
			uint32_t *somewhere = (uint32_t*)0x1A000000;
			*somewhere = *somewhere ^ 0xBABECAFE;
			asm volatile(
				".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			);
			HARTMAILBOX[1] = 0x0;
		}*/

		// Halt on wakeup
		asm volatile("wfi;");

		// We're signalled to run something
		/*if (OSScratchMem[hartid*NUMHARTWORDS+3] != 0x0)
		{
			OSScratchMem[hartid*NUMHARTWORDS+3] = 0;

			uint32_t progaddress = OSScratchMem[hartid*NUMHARTWORDS+2];
			asm volatile(
				"fence.i;"          // Invalidate I$
				"lw s0, %0;"        // Target branch address
				"jalr s0;"          // Branch to the entry point
				: "=m" (progaddress) : : 
			);
		}*/
	}
}

int main()
{
	// Before anything else happens, see if we have a "boot.elf" that we can transfer control to.
	{
		MountDrive();
		RunExecutable(0, "sd:boot.elf", false);
	}

	// No boot image found, fall back into a dummy OS

	// Diagnosis stage 0
	LEDSetState(0xFF);

	// Clear per-HART scratch memory in DDR3 memory (8Kbytes per HART)
	for (uint32_t i=0; i<NUM_HARTS*NUMHARTWORDS; i++)
		OSScratchMem[i] = 0x00000000;

	// Diagnosis stage 1
	LEDSetState(0xF0);

	// Install interrupt service routines
	// NOTE: Only after this point on timers / hw interrupts / illegal instruction exceptions start functioning
	InstallISR();

	// Diagnosis stage 2
	LEDSetState(0xAA);

	// Kick HART detection by sending a 'wake up' REQ to all HARTs except HART#0
	numdetectedharts = 1; // Self
	for (uint32_t i=1; i<NUM_HARTS; ++i)
	{
		HARTMAILBOX[i] = REQ_CheckAlive;
		HARTIRQ[i] = 1;
	}

	// Diagnosis stage 3
	LEDSetState(0x00);

	// Main loop, sleeps most of the time until an interrupt occurs
	while(1)
	{
		// Interrupt handler will do all the real work.
		// Therefore we can put the core to sleep until an interrupt occurs,
		// after which it will wake up to service it and then go back to
		// sleep, unless we asked it to crash.
		asm volatile("wfi;");

		// Process any keyboard events produced by the ISR and
		// parse the command generated in the command line on Enter.
		if (ProcessKeyEvents())
			ParseCommands();
	}

	return 0;
}
