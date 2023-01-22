#include "core.h"
#include "leds.h"
#include "xadc.h"
#include "uart.h"
#include "gpu.h"
#include "ps2.h"

#include <string.h>

// Keyboard map is at top of S-RAM (512 bytes)
uint16_t keymap[256];
// Previous key map to be able to track deltas (512 bytes)
uint16_t keymapprev[256];

void HandleMainTimer()
{
	// Splash screen
	UARTWrite("┌───────────────────────────────────────────┐\n");
	UARTWrite("│ non-OS v0.0014                            │\n");
	UARTWrite("│ (c)2022 Engin Cilasun                     │\n");
	UARTWrite("│                                           │\n");
	UARTWrite("│ Devices:                                  │\n");
	UARTWrite("│ DDR3: 0x00000000 - 0x1FFFFFFF (512Mbytes) │\n");
	UARTWrite("│ BMEM: 0x20000000 - 0x2003FFFF (256Kbytes) │\n");
	UARTWrite("│ UART: 0x80000000 - 0x8000000F : R/W       │\n");
	UARTWrite("│ LEDS: 0x80000010 - 0x8000001F : R/W       │\n");
	UARTWrite("│ XADC: 0x80000020 - 0x8000002F : R         │\n");
	UARTWrite("│ GPUC: 0x80000030 - 0x8000003F : W         │\n");
	UARTWrite("│ SPIC: 0x80000040 - 0x8000004F : R/W       │\n");
	UARTWrite("│ PS2C: 0x80000050 - 0x8000005F : R/W       │\n");
	UARTWrite("│                                           │\n");
	UARTWrite("│ Buffers/other:                            │\n");
	UARTWrite("│ PS2 ringbuffer: 0x2003FB00                │\n");
	UARTWrite("│ Stack top:      0x1FFFF000                │\n");
	UARTWrite("│ Heap:           0x04000000-0x1FFEFFFF     │\n");
	UARTWrite("└───────────────────────────────────────────┘\n\n");

	// This is currently a do-notthing timer handler ticking at 100ms
	// It will eventually become a task scheduler

	// Set up a time in far far future
	E32SetTimeCompare(0xFFFFFFFFFFFFFFFFULL);

	//uint64_t now = E32ReadTime();
	//uint64_t future = now + HUNDRED_MILLISECONDS_IN_TICKS;
	//E32SetTimeCompare(future);
}

void HandleUART()
{
	// Echo back all data in the FIFO
	while (UARTHasData())
	{
		uint8_t accu = UARTRead();
		// TODO: Stash as part of keyboard input
		*IO_UARTTX = accu;
	}
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
			//while(RingBufferWrite(&val, 4) == 0) { }
			// Ideally, this buffer should not overflow as long as the receiving app
			// is alive and well.
			PS2RingBufferWrite(&val, 4);
		}
	}
}

void HandleMainTrap(const uint32_t cause, const uint32_t a7, const uint32_t value, const uint32_t PC)
{
	// NOTE: One use of illegal instruction exception would be to do software emulation of the instruction in 'value'.
	switch (cause)
	{
		case CAUSE_BREAKPOINT:
		{
			// TODO: Setup and connect to debugger

			UARTWrite("\033[0m\n\n");
			UARTWrite("\033[31m\033[40m");
			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Debugger not implemented. Program halted.         │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");

			// Put core to endless sleep
			while(1) { asm volatile("wfi;"); }
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
			switch (a7)
			{
				case 96: UARTWrite("sys_exit();\n"); break;
				case 64: UARTWrite("sys_write();\n"); break;
				case 116: UARTWrite("sys_syslog();\n"); break;
				case 117: UARTWrite("sys_ptrace();\n"); break;
				default: UARTWrite("unknown();\n"); break;
			}

			UARTWrite("\033[0m\n\n");
			UARTWrite("\033[31m\033[40m");

			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Unimplemented M-ECALL. Program halted.            │\n");
			UARTWrite("│ #");
			UARTWriteHex((uint32_t)a7); // A7 containts Syscall ID
			UARTWrite(" @");
			UARTWriteHex((uint32_t)PC); // PC
			UARTWrite("                               │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");

			while(1) { asm volatile("wfi;"); } // Put core to endless sleep
		}
		break;

		//case CAUSE_MISALIGNED_FETCH:
		//case CAUSE_FETCH_ACCESS:

		case CAUSE_ILLEGAL_INSTRUCTION:
		{

		}
		break;

		//case CAUSE_MISALIGNED_LOAD:

		case CAUSE_LOAD_ACCESS:
		{
			UARTWrite("\033[0m\n\n");
			UARTWrite("\033[31m\033[40m");
			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Bus error: LOAD                                   │\n");
			UARTWrite("│ @");
			UARTWriteHex((uint32_t)PC); // PC
			UARTWrite("                                         │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");

			while(1) { asm volatile("wfi;"); } // Put core to endless sleep
		}
		break;

		//case CAUSE_MISALIGNED_STORE:

		case CAUSE_STORE_ACCESS:
		{
			UARTWrite("\033[0m\n\n");
			UARTWrite("\033[31m\033[40m");
			UARTWrite("┌───────────────────────────────────────────────────┐\n");
			UARTWrite("│ Bus error: STORE                                  │\n");
			UARTWrite("│ @");
			UARTWriteHex((uint32_t)PC); // PC
			UARTWrite("                                         │\n");
			UARTWrite("└───────────────────────────────────────────────────┘\n");
			UARTWrite("\033[0m\n");

			while(1) { asm volatile("wfi;"); } // Put core to endless sleep
		}
		break;

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

			while(1) { asm volatile("wfi;"); } // Put core to endless sleep
		}
		break; // Doesn't make sense but to make compiler happy...
	}
}

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) main_ISR()
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
		}

		if (code == 0x7) // Machine Timer Interrupt (timer)
		{
			HandleMainTimer();
		}
	}
	else // Machine Software Exception (trap)
	{
		HandleMainTrap(cause, a7, value, PC);
	}
}

void InstallMainISR()
{
	// Set machine trap vector
	swap_csr(mtvec, main_ISR);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + ONE_SECOND_IN_TICKS;
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Allow all machine interrupts to trigger
	swap_csr(mstatus, MSTATUS_MIE);
}

void InitSystem()
{
	memset(keymap, 0, 256*sizeof(uint16_t));
	memset(keymapprev, 0, 256*sizeof(uint16_t));

	InstallMainISR();
}
