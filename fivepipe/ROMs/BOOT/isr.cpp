#include "core.h"
#include "leds.h"
#include "uart.h"

void HandleMainTimer()
{
	// Roll LEDs
	static uint32_t ledstate = 0;
	SetLEDState(ledstate++);

	uint64_t now = E32ReadTime();
	uint64_t future = now + TEN_MILLISECONDS_IN_TICKS;
	E32SetTimeCompare(future);
}

void HandleMainTrap(const uint32_t cause, const uint32_t a7, const uint32_t value, const uint32_t PC)
{
	// NOTE: One use of illegal instruction exception would be to do software emulation of the instruction in 'value'.
	switch (cause)
	{
		case CAUSE_BREAKPOINT:
		{
			// TODO: Setup and connect to debugger
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
			UARTWrite("│ Unimplemented U/S/H/M ECALL. Program will resume  │\n");
			UARTWrite("│ execution, though it will most likely crash.      │\n");
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
			//if (value & 0x00000001) HandleUART();
			//if (value & 0x00000002) HandleButtons();
			//if (value & 0x00000004) HandleKeyboard();
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
