#include "encoding.h"
#include "core.h"
#include "leds.h"
#include "uart.h"
#include "task.h"
#include "isr.h"

#include <stdlib.h>

volatile static uint32_t g_currentTask = 0x0;
volatile static uint32_t g_taskcount = 0;
static STaskContext *g_taskctx = (STaskContext *)0x0;

void LEDBlinkyTask()
{
	// Invoked every 250 ms
	static uint32_t nextstate = 0;
	LEDSetState(nextstate++);
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

void HandleTimer()
{
	// Switch between running tasks
	TaskSwitchToTask(&g_currentTask, &g_taskcount, g_taskctx);

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
				HandleTimer();
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
}

void InstallISR()
{
	// Initialize task context memory
	g_taskctx = (STaskContext *)malloc(sizeof(STaskContext)*MAX_TASKS);
	TaskInitSystem(g_taskctx);

	// Set machine trap vector
	swap_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + TEN_MILLISECONDS_IN_TICKS; // One seconds into the future
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Add a test task to run every quarter of a second.
	g_taskcount = 0;
	TaskAdd(g_taskctx, &g_taskcount, LEDBlinkyTask, 512, ONE_SECOND_IN_TICKS/4);

	// Allow all machine interrupts to trigger (thus also enabling task system)
	swap_csr(mstatus, MSTATUS_MIE);
}