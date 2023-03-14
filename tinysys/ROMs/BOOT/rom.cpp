// Boot ROM

#include "rvcrt0.h"

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "core.h"
#include "fileops.h"
#include "isr.h"
#include "leds.h"
#include "uart.h"
#include "ringbuffer.h"

#define VERSIONSTRING "v0.95"

static char s_cmdString[512];
static int32_t s_cmdLen = 0;
static uint32_t s_startAddress = 0;

void _stubTask() {
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

void ExecuteCmd(char *_cmd)
{
	const char *command = strtok(_cmd, " ");
	if (!command)
		return;

	if (!strcmp(command, "dir"))
	{
		ListFiles("sd:\\");
	}
	else if (!strcmp(command, "cls"))
	{
		UARTWrite("\033[H\033[0m\033[2J");
	}
	else if (!strcmp(command, "ver"))
	{
		UARTWrite("tinysys " VERSIONSTRING "\r\n");
	}
	else if (!strcmp(command, "gdb"))
	{
		UARTWrite("\033[H\033[0m\033[2JEntering gdb debug server mode\r\n");
		TaskDebugMode(1);
	}
	else if (!strcmp(command, "help"))
	{
		UARTWrite("dir: Show list of files on sd:\\\r\n");
		UARTWrite("cls: Clear terminal\r\n");
		UARTWrite("gdb: Enter gdb server mode\r\n");
		UARTWrite("ver: Show version info\r\n");
		UARTWrite("Any other input will load a file from sd: with matching name\r\n");
		UARTWrite("CTRL+C terminates the current program\r\n");
	}
	else // Anything else defers to being a command on storage
	{
		char filename[128];
		strcpy(filename, "sd:\\");
		strcat(filename, command);
		strcat(filename, ".elf");

		// First parameter is excutable name
		s_startAddress = LoadExecutable(filename, true);

		// If we succeeded in loading the executable, the trampoline task can branch into it.
		// NOTE: Without code relocation or virtual memory, two executables will overwrite when loaded
		// even though each gets a new task.
		if (s_startAddress != 0x0)
			TaskAdd(GetTaskContext(), command, RunExecTask, HUNDRED_MILLISECONDS_IN_TICKS);
	}
}

int main()
{
	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2Jtinysys " VERSIONSTRING "\r\nType 'help' for CLI usage info\r\n\r\n");

	// Set up internals
	RingBufferReset();

	// Mount the FAT volume on micro sd card
	MountDrive();

	// Create task context
	STaskContext *taskctx = CreateTaskContext();

	// With current layout, OS takes up a very small slices out of whatever is left from other tasks
	TaskAdd(taskctx, "_stub", _stubTask, HALF_MILLISECOND_IN_TICKS);

	// Start the timer and hardware interrupt handlers.
	// This is where all task switching and other interrupt handling occurs
	// TODO: Software debugger will run here in the UART interrupt handler
	// and modify task memory to aid in debugging via gdb serial interface.
	InstallISR();

	uint64_t past = 0;
	uint32_t evenodd = 0;
	int stringchanged = 1;
	while (1) {

		uint64_t present = E32ReadTime();
		// Swap LED state roughtly every other second
		if (present-past > ONE_SECOND_IN_TICKS)
		{
			past += ONE_SECOND_IN_TICKS; // Preserve leftover difference in present
			LEDSetState((evenodd%2==0) ? 0xFFFFFFFF : 0x00000000); // Toggle all LEDs on/off
			++evenodd;
		}

		// Echo all of the characters we can find back to the sender
		uint32_t uartData = 0;
		int execcmd = 0;
		while (RingBufferRead(&uartData, sizeof(uint32_t)))
		{
			uint8_t asciicode = (uint8_t)(uartData&0xFF);
			stringchanged++;
			switch (asciicode)
			{
				case 10:
				case 13:	// Return / Enter
				{
					execcmd++;
				}
				break;

				case 3:		// EXT (Ctrl+C)
				{
					// TODO: Terminate process 1 (the executable)
					TaskExitTaskWithID(taskctx, 1, 0); // Sig:0
				}

				case 8:		// Backspace
				{
					s_cmdLen--;
					if (s_cmdLen<0)
						s_cmdLen = 0;
				}
				break;

				case 27:	// ESC
				{
					s_cmdLen = 0;
					// TODO: Erase current line
				}
				break;

				default:
				{
					s_cmdString[s_cmdLen++] = (char)asciicode;
					if (s_cmdLen > 511)
						s_cmdLen = 511;
				}
				break;
			}
		}

		if (stringchanged)
		{
			stringchanged = 0;
			s_cmdString[s_cmdLen] = 0;
			// Reset current line and emit the command string
			UARTWrite("\033[2K\rsd:\\");
			UARTWrite(s_cmdString);
		}

		if (execcmd)
		{
			stringchanged = 1;
			UARTWrite("\r\n");
			ExecuteCmd(s_cmdString);
			// Rewind
			s_cmdLen = 0;
			s_cmdString[0] = 0;
		}
	}

	return 0;
}
