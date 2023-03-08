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
	/*asm volatile (
		"mv tp, sp;"		// Save current stack pointer
		"addi sp,sp,-112;"	// Make space
		"sw ra,0(sp);"		// Save all registers
		"sw tp,4(sp);"
		"sw t0,8(sp);"
		"sw t1,12(sp);"
		"sw t2,16(sp);"
		"sw a0,20(sp);"
		"sw a1,24(sp);"
		"sw a2,28(sp);"
		"sw a3,32(sp);"
		"sw a4,36(sp);"
		"sw a5,40(sp);"
		"sw a6,44(sp);"
		"sw a7,48(sp);"
		"sw t3,52(sp);"
		"sw t4,56(sp);"
		"sw t5,60(sp);"
		"sw t6,64(sp);"
		"sw s0,68(sp);"
		"sw s1,72(sp);"
		"sw s2,76(sp);"
		"sw s3,80(sp);"
		"sw s4,84(sp);"
		"sw s5,88(sp);"
		"sw s6,92(sp);"
		"sw s7,96(sp);"
		"sw s8,100(sp);"
		"sw s9,104(sp);"
		"sw s10,108(sp);"
		"sw s11,112(sp);"
		//"sw gp,114(sp);" 	// Do not save/restore global pointer across tasks
	);*/

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

				// Task scheduler resolution is 100 ms
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

	/*asm volatile (
		"sw ra,0(sp);"		// Save all registers
		"sw tp,4(sp);"
		"sw t0,8(sp);"
		"sw t1,12(sp);"
		"sw t2,16(sp);"
		"sw a0,20(sp);"
		"sw a1,24(sp);"
		"sw a2,28(sp);"
		"sw a3,32(sp);"
		"sw a4,36(sp);"
		"sw a5,40(sp);"
		"sw a6,44(sp);"
		"sw a7,48(sp);"
		"sw t3,52(sp);"
		"sw t4,56(sp);"
		"sw t5,60(sp);"
		"sw t6,64(sp);"
		"sw s0,68(sp);"
		"sw s1,72(sp);"
		"sw s2,76(sp);"
		"sw s3,80(sp);"
		"sw s4,84(sp);"
		"sw s5,88(sp);"
		"sw s6,92(sp);"
		"sw s7,96(sp);"
		"sw s8,100(sp);"
		"sw s9,104(sp);"
		"sw s10,108(sp);"
		"sw s11,112(sp);"
		//"sw gp,114(sp);" 	// Do not save/restore global pointer across tasks
		"mv sp,tp;"			// Restore stack pointer
		"mret;"
	);*/
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