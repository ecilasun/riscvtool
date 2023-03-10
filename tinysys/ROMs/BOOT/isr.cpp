#include "encoding.h"
#include "core.h"
#include "debugger.h"
#include "uart.h"
#include "isr.h"

#include <stdlib.h>

static STaskContext *g_taskctx = (STaskContext *)0x0;

STaskContext *StartTaskContext()
{
	// Initialize task context memory
	g_taskctx = (STaskContext *)malloc(sizeof(STaskContext));
	TaskInitSystem(g_taskctx);

	return g_taskctx;
}

void HandleUART()
{

	// XOFF - hold data traffic from sender so that we don't get stuck here
	// *IO_UARTTX = 0x13;

	// Echo back incoming bytes
	while (UARTInputFifoHasData())
	{
		// Consume incoming character
		uint8_t incoming = *IO_UARTRX;
		// Echo back
		*IO_UARTTX = incoming;
	}

	// XON - resume data traffic from sender
	// *IO_UARTTX = 0x11;
}

//void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) interrupt_service_routine() // Auto-saves registers
void __attribute__((aligned(16))) __attribute__((naked)) interrupt_service_routine() // Manual register save
{
	// Use extra space in CSR file to store a copy of the current register set before we overwrite anything
	// TODO: Used vectored interrupts so we don't have to do this for all kinds of interrupts
//		csrw 0xFE3, tp;
	asm volatile(" \
		csrw 0xFE0, ra; \
		csrw 0xFE1, sp; \
		csrw 0xFE2, gp; \
		csrw 0xFE4, t0; \
		csrw 0xFE5, t1; \
		csrw 0xFE6, t2; \
		csrw 0xFE7, s0; \
		csrw 0xFE8, s1; \
		csrw 0xFE9, a0; \
		csrw 0xFEA, a1; \
		csrw 0xFEB, a2; \
		csrw 0xFEC, a3; \
		csrw 0xFED, a4; \
		csrw 0xFEE, a5; \
		csrw 0xFEF, a6; \
		csrw 0xFF0, a7; \
		csrw 0xFF1, s2; \
		csrw 0xFF2, s3; \
		csrw 0xFF3, s4; \
		csrw 0xFF4, s5; \
		csrw 0xFF5, s6; \
		csrw 0xFF6, s7; \
		csrw 0xFF7, s8; \
		csrw 0xFF8, s9; \
		csrw 0xFF9, s10; \
		csrw 0xFFA, s11; \
		csrw 0xFFB, t3; \
		csrw 0xFFC, t4; \
		csrw 0xFFD, t5; \
		csrw 0xFFE, t6; \
		csrr a5, mepc; \
		csrw 0xFFF, a5; \
	");

	// CSR[0xFF0] now contains A7 (SYSCALL number)

	//uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	//uint32_t PC = read_csr(mepc)-4;   // Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000) // Hardware interrupts
	{
		switch (code)
		{
			case IRQ_M_EXT:
			{
				// TODO: We need to read the 'reason' for HW interrupt from a custom register
				// For now, ignore the reason as we only have one possible IRQ generator.
				//gdb_handler(g_taskctx);
				HandleUART();

				// Machine External Interrupt (hardware)
				// Route based on hardware type
				//if (value & 0x00000001) HandleUART();
				//if (value & 0x00000002) HandleKeyboard();
				//if (value & 0x00000004) HandleJTAG();
			}
			break;

			case IRQ_M_TIMER:
			{
				// Machine Timer Interrupt (timer)
				// Task scheduler runs here

				// Switch between running tasks
				TaskSwitchToNext(g_taskctx);

				// Task scheduler will re-visit in 100ms
				uint64_t now = E32ReadTime();
				uint64_t future = now + HUNDRED_MILLISECONDS_IN_TICKS;
				E32SetTimeCompare(future);
			}
			break;

			default:
			break;
		}
	}
	else // Exceptions
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
				//HandleTrap(cause, a7, value, PC); // Illegal instruction etc
			}
			break;

			default: break;
		}
	}

	// Restore registers to next task's register set
	// TODO: Used vectored interrupts so we don't have to do this for all kinds of interrupts
//		csrr tp, 0xFE3;
	asm volatile(" \
		csrr ra, 0xFE0; \
		csrr sp, 0xFE1; \
		csrr gp, 0xFE2; \
		csrr t0, 0xFE4; \
		csrr t1, 0xFE5; \
		csrr t2, 0xFE6; \
		csrr s0, 0xFE7; \
		csrr s1, 0xFE8; \
		csrr a0, 0xFE9; \
		csrr a1, 0xFEA; \
		csrr a2, 0xFEB; \
		csrr a3, 0xFEC; \
		csrr a4, 0xFED; \
		csrr a5, 0xFEE; \
		csrr a6, 0xFEF; \
		csrr a7, 0xFF0; \
		csrr s2, 0xFF1; \
		csrr s3, 0xFF2; \
		csrr s4, 0xFF3; \
		csrr s5, 0xFF4; \
		csrr s6, 0xFF5; \
		csrr s7, 0xFF6; \
		csrr s8, 0xFF7; \
		csrr s9, 0xFF8; \
		csrr s10, 0xFF9; \
		csrr s11, 0xFFA; \
		csrr t3, 0xFFB; \
		csrr t4, 0xFFC; \
		csrr t5, 0xFFD; \
		csrr t6, 0xFFE; \
		csrr a5, 0xFFF; \
		csrw mepc, a5; \
		mret; \
	");
}

void InstallISR()
{
	// Set machine trap vector
	write_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + HUNDRED_MILLISECONDS_IN_TICKS;
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	write_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Allow all machine interrupts to trigger (thus also enabling task system)
	write_csr(mstatus, MSTATUS_MIE);
}