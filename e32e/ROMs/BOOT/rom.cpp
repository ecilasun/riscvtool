// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"
#include "elf.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "buttons.h"
#include "ps2.h"
#include "ringbuffer.h"
#include "gpu.h"
//#include "debugger.h"

#define REQ_CheckAlive			0x00000000
#define REQ_InstallTISR			0x00000001
#define REQ_StartExecutable		0x00000002

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

static char currentdir[512] = "sd:";
static char commandline[512] = "";
static char filename[128] = "";
static int cmdlen = 0;
static int havedrive = 0;
static uint32_t numdetectedharts = 0;
static int defaultHardID = 0;

// Space for per-hart data
static uint32_t *hartData = nullptr;

// Main file system object
FATFS Fs;

// Keyboard event ring buffer (1024 bytes)
uint8_t *keyboardringbuffer = (uint8_t*)0x80000200; // 512 bytes into mailbox memory
// Keyboard map is at top of S-RAM (512 bytes)
uint16_t keymap[256];
// Previous key map to be able to track deltas (512 bytes)
uint16_t keymapprev[256];

void HandleUART()
{
	//gdb_handler(task_array, num_tasks);

	//*IO_UARTRXTX = 0x13; // XOFF
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

	//*IO_UARTRXTX = 0x11; // XON
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
	// Report live HART count
	for (uint32_t i=1; i<NUM_HARTS; ++i)
		if (HARTMAILBOX[i] == 0xE32EBABE) // This is an E32E core, consider alive
			numdetectedharts++;

	UARTWrite("\n\033[34m\033[47m\033[7m");
	UARTWrite("Number of HARTs detected: ");
	UARTWriteDecimal(numdetectedharts);
	UARTWrite("\033[0m\n");

	// Stop further timer interrupts by setting timecmp to furthest value available.
	swap_csr(0x801, 0xFFFFFFFF);
	swap_csr(0x800, 0xFFFFFFFF);
}

void HandleKeyboard()
{
	// Consume all key state changes from FIFO and update the key map
	while (*PS2KEYBOARDDATAAVAIL)
		ScanKeyboard(keymap);

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
			// TODO: Debugger related
		}
		break;

		/*case CAUSE_USER_ECALL:
		case CAUSE_SUPERVISOR_ECALL:
		case CAUSE_HYPERVISOR_ECALL:*/
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

		/*case CAUSE_MISALIGNED_FETCH:
		case CAUSE_FETCH_ACCESS:
		case CAUSE_ILLEGAL_INSTRUCTION:
		case CAUSE_MISALIGNED_LOAD:
		case CAUSE_LOAD_ACCESS:
		case CAUSE_MISALIGNED_STORE:
		case CAUSE_STORE_ACCESS:
		case CAUSE_FETCH_PAGE_FAULT:
		case CAUSE_LOAD_PAGE_FAULT:
		case CAUSE_STORE_PAGE_FAULT:*/
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
	uint64_t future = now + ONE_SECOND_IN_TICKS; // One second into the future (based on 10MHz wall clock)
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
			hartData[hartid*NUMHARTWORDS+0] = TISR;
			hartData[hartid*NUMHARTWORDS+1] = TISR_Interval;

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
			uint32_t progaddress = HARTMAILBOX[hartid*HARTPARAMCOUNT+0+NUM_HARTS];
			hartData[hartid*NUMHARTWORDS+2] = progaddress;
			hartData[hartid*NUMHARTWORDS+3] = 0x1;
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
			/*if (value & 0x00000001) HandleUART();
			if (value & 0x00000002) HandleButtons();
			if (value & 0x00000004) HandleKeyboard();*/

			// Service request handler
			if (value & 0x00000010) HandleHARTIRQ(hartid);
		}

		if (code == 0x7) // Machine Timer Interrupt (timer)
		{
			uint32_t keepAlive = 0;
			// Chain into the TISR if there is one installed
			uint32_t TISR = hartData[hartid*NUMHARTWORDS+0];

			// If there's an installed routine, call it
			if (TISR)
				keepAlive = ((t_timerISR)TISR)(hartid);

			if (keepAlive) // TISR wishes to stay alive for next time
			{
				// Re-set the timer interval to wake up again
				uint32_t TISR_Interval = hartData[hartid*NUMHARTWORDS+1];
				uint64_t now = E32ReadTime();
				uint64_t future = now + TISR_Interval;
				E32SetTimeCompare(future);
			}
			else // TISR wishes to be terminated
			{
				// Uninstall the routine
				hartData[hartid*NUMHARTWORDS+0] = 0x0;
				hartData[hartid*NUMHARTWORDS+1] = 0x0;

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
	/*uint64_t now = E32ReadTime();
	uint64_t future = now + 512; // Set to happen very soon, around similar points in time for all HARTs except #0
	E32SetTimeCompare(future);*/

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

void CLF()
{
	// Write to FB0, display FB1
	*GPUCTL = 0;
	for (int y=0;y<240;++y)
		for (int x=0;x<80;++x)
		GPUFBWORD[x+y*80] = 0x00000000;

	// Write to FB1, display FB0
	*GPUCTL = 1;
	for (int y=0;y<240;++y)
		for (int x=0;x<80;++x)
		GPUFBWORD[x+y*80] = 0x00000000;

	// Back to displaying FB1
	*GPUCTL = 0;
}

void CLS()
{
	UARTWrite("\033[0m\033[2J");
}

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
			/*UARTWriteHex(sheader.m_Addr);
			UARTWrite(" @");
			UARTWriteHex(sheader.m_Offset);
			UARTWrite(" ");
			UARTWriteHex(sheader.m_Size);
			UARTWrite("\n");*/
			// ...place it in memory
			uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
			f_lseek(fp, sheader.m_Offset);
			f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
		}
	}

	free(names);
}

void RunExecutable(const int hartID, const char *filename)
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

		if (hartID == 0)
		{
			// Unmount filesystem and reset to root directory before passing control
			f_mount(nullptr, "sd:", 1);
			strcpy(currentdir, "sd:");
			havedrive = 0;
		}

		// Run the executable on HART#0
		if (hartID == 0)
		{
			asm volatile(
				".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
				"fence.i;"          // Invalidate I$
				"lw s0, %0;"        // Target branch address
				"jalr s0;"          // Branch to the entry point
				: "=m" (branchaddress) : : 
			);
		}
		else
		{
			asm volatile(
				".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			);

			// Use a REQ# to get another core to boot the executable
			HARTMAILBOX[hartID] = REQ_StartExecutable;
			HARTMAILBOX[hartID*HARTPARAMCOUNT+0+NUM_HARTS] = branchaddress; // Executable is at this address
			HARTIRQ[hartID] = 1;
		}

		if (hartID == 0)
		{
			// Re-mount filesystem before re-gaining control
			f_mount(&Fs, "sd:", 1);
			havedrive = 1;
		}
	}
	else
	{
		UARTWrite("Executable '");
		UARTWrite(filename);
		UARTWrite("' not found.\n");
	}
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
		UARTWrite("\033[34m\033[47m\033[7mcls\033[0m: clear visible portion of terminal\n");
		UARTWrite("\033[34m\033[47m\033[7mhart\033[0m: set hart to run loaded executable on\n");
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
	else if (!strcmp(command, "cls")) // Clear terminal screen
	{
		CLS();
	}
	else if (!strcmp(command, "hart")) // Set hart to run the loaded executable
	{
		char *param = strtok(nullptr, " ");
		if (param != nullptr)
			defaultHardID = atoi(param);
		else
			defaultHardID = 0;
	}
	else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
	{
		// Build a file name from the input string
		strcpy(filename, currentdir);
		strcat(filename, command);
		strcat(filename, ".elf");
		RunExecutable(defaultHardID, filename);
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

// Worker CPU entry point
void workermain()
{
	InstallWorkerISR(); // In a very short while, this will trigger a timer interrupt to test 'alive'

	uint32_t hartid = read_csr(mhartid);

	while (1)
	{
		// Halt on wakeup
		asm volatile("wfi;");

		// We're signalled to run something
		if (hartData[hartid*NUMHARTWORDS+3] != 0x0)
		{
			hartData[hartid*NUMHARTWORDS+3] = 0;

			uint32_t progaddress = hartData[hartid*NUMHARTWORDS+2];
			asm volatile(
				"fence.i;"          // Invalidate I$
				"lw s0, %0;"        // Target branch address
				"jalr s0;"          // Branch to the entry point
				: "=m" (progaddress) : : 
			);
		}
	}
}

int main()
{
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
	{
		havedrive = 0;
		UARTWrite(FRtoString[mountattempt]);
	}
	else
		havedrive = 1;

	// If there's an autoexec.elf on the SDCard, boot that one instead
	// before we touch any of the hardware
	if (havedrive)
		RunExecutable(0, "autoexec.elf"); // Boot on HART#0

	// Set up per-HART scratch memory
	hartData = (uint32_t*)0x1FFF0000; // enough space for 4 of sizeof(uint32_t)*NUM_HARTS*NUMHARTWORDS;

	// Interrupt service routine
	InstallISR();

	// Kick HART detection by seinding a 'wake up' to all HARTs except this one.
	numdetectedharts = 1;
	for (uint32_t i=1; i<NUM_HARTS; ++i)
	{
		HARTMAILBOX[i] = REQ_CheckAlive;
		HARTIRQ[i] = 1;
	}

	// Clear framebuffers
	CLF();

	// Clear all attributes, clear screen, print boot message
	CLS();
	UARTWrite("┌────────────────────────────────────┐\n");
	UARTWrite("│ E32OS v0.162 (c)2022 Engin Cilasun │\n");
	UARTWrite("└────────────────────────────────────┘\n\n");

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
