// Boot ROM

#include "rvcrt0.h"

//#include "memtest/memtest.h"

#include <math.h>
#include <string.h>

#include "core.h"
#include "fileops.h"
#include "isr.h"
#include "leds.h"
#include "uart.h"

static uint32_t s_startAddress = 0;

void OSIdleTask()
{
	// NOTE: This task should not run
	// First time we enter the ISR it will be stomped over by main()
	while(1)
	{
		asm volatile("nop;");
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
	UARTWrite("task[0].PC=");
	UARTWriteHex(taskctx->tasks[0].regs[0]);
	UARTWrite("\ntask[1].PC=");
	UARTWriteHex(taskctx->tasks[1].regs[0]);
	UARTWrite("\n");

	// Start the timer and hardware interrupt handlers
	InstallISR();

	uint64_t past = 0;
	uint32_t evenodd = 0;
	while (1) {
		uint64_t present = E32ReadTime();
		if (present-past > ONE_SECOND_IN_TICKS)
		{
			past = present;
			LEDSetState((evenodd%2==0) ? 0xFFFFFFFF : 0x00000000);
			++evenodd;
		}
	}

	return 0;
}
