#include "e32os.h"
#include "workercore.h"

void HandleHARTIRQ(const uint32_t hartid)
{
	// Immediately stop to avoid further HART interrupts
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
			//if (value & 0x00000002) HandleButtons();
			//if (value & 0x00000004) HandleKeyboard();
			if (value & 0x00000010) HandleHARTIRQ(hartid);
		}
		else if (code == 0x7) // Machine Timer Interrupt (timer)
		{
			swap_csr(0x801, 0xFFFFFFFF);
			swap_csr(0x800, 0xFFFFFFFF);
		}
	}
	else // Machine Software Exception (trap)
	{
		//HandleTrap(cause, a7, value, PC);
	}
}

void InstallWorkerCoreISR()
{
	// Set machine trap vector
	swap_csr(mtvec, worker_interrupt_service_routine);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Allow all machine interrupts to trigger
	swap_csr(mstatus, MSTATUS_MIE);
}
