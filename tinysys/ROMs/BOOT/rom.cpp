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

// NOTE: This task won't actually run.
// First time we enter the ISR it will be stomped over by whatever the MRET was inside main()
void OSMainStubTask() { asm volatile("nop;"); }

// This task is a trampoline to the loaded executable
void RunExecTask()
{
	// Start the loaded executable
	asm volatile(
		"lw s0, %0;"        // Target branch address
		"jalr s0;"          // Branch to the entry point
		: "=m" (s_startAddress) : : 
	);
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

	// With current layout, OS takes up ten millisecond slices out of whatever is left from other tasks
	TaskAdd(taskctx, "OSMain", OSMainStubTask, TEN_MILLISECONDS_IN_TICKS);
	TaskAdd(taskctx, "BootTask", RunExecTask, TWO_HUNDRED_FIFTY_MILLISECONDS_IN_TICKS);

	// Start the timer and hardware interrupt handlers
	InstallISR();

	uint64_t past = 0;
	uint32_t evenodd = 0;
	while (1) {
		// Main loop gets 10ms run time every 250ms which is more than enough for very slow periodic tasks
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
