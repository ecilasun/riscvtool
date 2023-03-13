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

void OSMainStubTask() {
	// NOTE: This task won't actually run
	// It is a stub routine which will be stomped over by main()
	// on first entry to the timer ISR
	asm volatile("nop;");
}

// This task is a trampoline to the loaded executable
void RunExecTask()
{
	// Start the loaded executable
	asm volatile(
		"lw s0, %0;"        // Target branch address
		"jalr s0;"          // Branch to the entry point
		: "=m" (s_startAddress) : : 
	);

	// NOTE: Execution should never reach here since the ELF will invoke ECALL(0x5D) to quit
	// and will be removed from the task list, thus removing this function from the
	// execution pool.
}

int main()
{
	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2Jtinysys v0.8\n");

	// Load startup executable
	MountDrive();
	s_startAddress = LoadExecutable("sd:\\boot.elf", true);

	// Create task context
	STaskContext *taskctx = CreateTaskContext();

	// With current layout, OS takes up a very small slices out of whatever is left from other tasks
	TaskAdd(taskctx, "OSMain", OSMainStubTask, ONE_MILLISECOND_IN_TICKS);
	// If we succeeded in loading the boot executable, add a trampoline function to launch it
	if (s_startAddress != 0x0)
		TaskAdd(taskctx, "BootTask", RunExecTask, TWO_HUNDRED_FIFTY_MILLISECONDS_IN_TICKS);

	// Start the timer and hardware interrupt handlers.
	// This is where all task switching and other interrupt handling occurs
	// TODO: Software debugger will run here in the UART interrupt handler
	// and modify task memory to aid in debugging via gdb serial interface.
	InstallISR();

	uint64_t past = 0;
	uint32_t evenodd = 0;
	while (1) {
		uint64_t present = E32ReadTime();
		if (present-past > ONE_SECOND_IN_TICKS)
		{
			// Swap LED state roughtly every other second
			// We have a 10ms window for this which is a whole lot more than enough
			past = present;
			LEDSetState((evenodd%2==0) ? 0xFFFFFFFF : 0x00000000);
			++evenodd;
		}
	}

	return 0;
}
