#include "encoding.h"
#include "core.h"
#include "debugger.h"
#include "leds.h"
#include "uart.h"
#include "task.h"
#include "isr.h"

#include <stdlib.h>

static STaskContext *g_taskctx = (STaskContext *)0x0;

void OSIdleTask()
{
	while(1)
	{
		asm volatile("nop;");
	}
}

void LEDBlinkyTask()
{
	static uint32_t nextstate = 0;
	static uint64_t prevTime = 0;
	while(1)
	{
		uint64_t now = E32ReadTime();
		if (now - prevTime > ONE_SECOND_IN_TICKS)
		{
			prevTime = now;
			// Advance LED state approximately every second
			LEDSetState(nextstate++);
		}
	}
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

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) interrupt_service_routine() // Auto-saves registers
{
	// Use extra space in CSR file to store a copy of the current register set before we overwrite anything
	// TODO: Used vectored interrupts so we don't have to do this for all kinds of interrupts
	/*asm volatile(" \
		csrrw zero, 0xFE0, ra; \
		csrrw zero, 0xFE1, sp; \
		csrrw zero, 0xFE2, gp; \
		csrrw zero, 0xFE3, tp; \
		csrrw zero, 0xFE4, t0; \
		csrrw zero, 0xFE5, t1; \
		csrrw zero, 0xFE6, t2; \
		csrrw zero, 0xFE7, s0; \
		csrrw zero, 0xFE8, s1; \
		csrrw zero, 0xFE9, a0; \
		csrrw zero, 0xFEA, a1; \
		csrrw zero, 0xFEB, a2; \
		csrrw zero, 0xFEC, a3; \
		csrrw zero, 0xFED, a4; \
		csrrw zero, 0xFEE, a5; \
		csrrw zero, 0xFEF, a6; \
		csrrw zero, 0xFF0, a7; \
		csrrw zero, 0xFF1, s2; \
		csrrw zero, 0xFF2, s3; \
		csrrw zero, 0xFF3, s4; \
		csrrw zero, 0xFF4, s5; \
		csrrw zero, 0xFF5, s6; \
		csrrw zero, 0xFF6, s7; \
		csrrw zero, 0xFF7, s8; \
		csrrw zero, 0xFF8, s9; \
		csrrw zero, 0xFF9, s10; \
		csrrw zero, 0xFFA, s11; \
		csrrw zero, 0xFFB, t3; \
		csrrw zero, 0xFFC, t4; \
		csrrw zero, 0xFFD, t5; \
		csrrw zero, 0xFFE, t6; \
		csrr a5, mepc; \
		csrrw zero, 0xFFF, a5;");*/

	//uint32_t a7;
	//asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined (this is the SYSCALL function code)

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
	/*asm volatile(" \
		csrrw a5, 0xFFF, zero; \
		csrrw zero, mepc, a5; \
		csrrw ra, 0xFE0, zero; \
		csrrw sp, 0xFE1, zero; \
		csrrw gp, 0xFE2, zero; \
		csrrw tp, 0xFE3, zero; \
		csrrw t0, 0xFE4, zero; \
		csrrw t1, 0xFE5, zero; \
		csrrw t2, 0xFE6, zero; \
		csrrw s0, 0xFE7, zero; \
		csrrw s1, 0xFE8, zero; \
		csrrw a0, 0xFE9, zero; \
		csrrw a1, 0xFEA, zero; \
		csrrw a2, 0xFEB, zero; \
		csrrw a3, 0xFEC, zero; \
		csrrw a4, 0xFED, zero; \
		csrrw a5, 0xFEE, zero; \
		csrrw a6, 0xFEF, zero; \
		csrrw a7, 0xFF0, zero; \
		csrrw s2, 0xFF1, zero; \
		csrrw s3, 0xFF2, zero; \
		csrrw s4, 0xFF3, zero; \
		csrrw s5, 0xFF4, zero; \
		csrrw s6, 0xFF5, zero; \
		csrrw s7, 0xFF6, zero; \
		csrrw s8, 0xFF7, zero; \
		csrrw s9, 0xFF8, zero; \
		csrrw s10, 0xFF9, zero; \
		csrrw s11, 0xFFA, zero; \
		csrrw t3, 0xFFB, zero; \
		csrrw t4, 0xFFC, zero; \
		csrrw t5, 0xFFD, zero; \
		csrrw t6, 0xFFE, zero;");*/
}

void InstallISR()
{
	// Initialize task context memory
	g_taskctx = (STaskContext *)malloc(sizeof(STaskContext));
	TaskInitSystem(g_taskctx);

	// Set machine trap vector
	swap_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + HUNDRED_MILLISECONDS_IN_TICKS; // One seconds into the future
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Add a test task to run every quarter of a second.
	TaskAdd(g_taskctx, "OSIdle", OSIdleTask, 512, ONE_SECOND_IN_TICKS/4);
	TaskAdd(g_taskctx, "Blinky", LEDBlinkyTask, 512, ONE_SECOND_IN_TICKS/4);

	// Allow all machine interrupts to trigger (thus also enabling task system)
	swap_csr(mstatus, MSTATUS_MIE);
}