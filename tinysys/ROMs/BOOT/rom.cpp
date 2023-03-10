// Boot ROM

#include "rvcrt0.h"

//#include "memtest/memtest.h"

#include <math.h>
#include <string.h>

#include "core.h"
#include "fileops.h"
#include "uart.h"
#include "isr.h"

static uint32_t s_startAddress = 0;

void OSIdleTask()
{
	static uint32_t past = 0;
	while(1)
	{
		uint64_t present = E32ReadTime();
		if (present-past > ONE_SECOND_IN_TICKS)
		{
			past = present;
			UARTWrite(".");
			asm volatile("nop;");
		}
	}
}

void RunExecTask()
{
	UARTWrite("Running...\n");

	// Start the loaded executable
	RunExecutable(s_startAddress);
}

int main()
{
	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2Jtinysys v0.8\n");

	// Load startup executable
	MountDrive();
	s_startAddress = LoadExecutable("sd:\\boot.elf", true);

	// Start task sytem and add tasks
	// NOTE: First task should be OSIdle to replace the main loop here.
	STaskContext *taskctx = StartTaskContext();
	TaskAdd(taskctx, "OSIdle", OSIdleTask);
	TaskAdd(taskctx, "BootTask", RunExecTask);

	// Start the timer and hardware interrupt handlers
	InstallISR();

	while (1) {
		asm volatile ("nop;");
	}

	return 0;
}
