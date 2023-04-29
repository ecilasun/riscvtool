// Boot ROM

#include "rvcrt0.h"
#include "rombase.h"
#include "xadc.h"

#include <string.h>
#include <stdio.h>

#define VERSIONSTRING "v1.001"

static char s_execName[64];
static char s_execParam[64];
static char s_cmdString[128];
static char s_currentPath[64];
static uint32_t s_execParamCount = 1;
static int32_t s_cmdLen = 0;
static uint32_t s_startAddress = 0;
static int s_refreshConsoleOut = 1;
static int s_driveMounted = 0;

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
		"addi sp, sp, -16;"
		"sw %3, 0(sp);"		// Store argc
		"sw %1, 4(sp);"		// Store argv[1] (path to exec)
		"sw %2, 8(sp);"		// Store argv[2] (exec params)
		"sw zero, 12(sp);"	// Store argv[3] (nullptr)
		"lw s0, %0;"        // Target branch address
		"jalr s0;"          // Branch to the entry point
		"addi sp, sp, 16;"	// We most likely won't return here
		: "=m" (s_startAddress)
		: "r" (s_execName), "r" (s_execParam), "r" (s_execParamCount)
		// Clobber list
		: "s0"
	);

	// NOTE: Execution should never reach here since the ELF will invoke ECALL(0x5D) to quit
	// and will be removed from the task list, thus removing this function from the
	// execution pool.
}

void uget(const char *savename)
{
	// TODO: Load file from remote over UART
	if (!savename)
		UARTWrite("usage: uget targetfilename\n");
	else
		UARTWrite("TODO: Add task to receive files\n");
}

void ExecuteCmd(char *_cmd)
{
	const char *command = strtok(_cmd, " ");
	if (!command)
		return;

	if (!strcmp(command, "dir"))
	{
		ListFiles(s_currentPath);
	}
	else if (!strcmp(command, "cls"))
	{
		UARTWrite("\033[H\033[0m\033[2J");
	}
	else if (!strcmp(command, "mem"))
	{
		UARTWrite("Available memory:");
		uint32_t inkbytes = core_memavail()/1024;
		uint32_t inmbytes = inkbytes/1024;
		if (inmbytes!=0)
		{
			UARTWriteDecimal(inmbytes);
			UARTWrite(" Mbytes\n");
		}
		else
		{
			UARTWriteDecimal(inkbytes);
			UARTWrite(" Kbytes\n");
		}
	}
	else if (!strcmp(command, "prc"))
	{
		STaskContext *ctx = GetTaskContext();
		for (int i=1;i<ctx->numTasks;++i)
		{
			STask *task = &ctx->tasks[i];
			// UARTWrite("#");
			// UARTWriteDecimal(i);
			UARTWrite(" task:");
			UARTWrite(task->name);
			UARTWrite(" len:");
			UARTWriteDecimal(task->runLength);
			UARTWrite(" state:");
			UARTWriteDecimal(task->state);
			UARTWrite(" PC:");
			UARTWriteHex(task->regs[0]);
			UARTWrite("\n");
		}
	}
	else if (!strcmp(command, "del"))
	{
		const char *path = strtok(nullptr, " ");
		// TODO: delete a file
		if (!path)
			UARTWrite("usage: del fname\n");
		else
			remove(path);
	}
	else if (!strcmp(command, "cwd"))
	{
		const char *path = strtok(nullptr, " ");
		// Change working directory
		if (!path)
			UARTWrite("usage: cwd path\n");
		else
			strncpy(s_currentPath, path, 128);
	}
	else if (!strcmp(command, "get"))
	{
		const char *savename = strtok(nullptr, " ");
		uget(savename);
	}
	else if (!strcmp(command, "ver"))
	{
		UARTWrite("tinysys " VERSIONSTRING "\n");
	}
	else if (!strcmp(command, "gdb"))
	{
		UARTWrite("\033[H\033[0m\033[2JEntering gdb debug server mode\n");
		TaskDebugMode(1);
	}
	else if (!strcmp(command, "tmp"))
	{
		UARTWrite("Temperature:");
		uint32_t ADCcode = *XADCTEMP;
		float temp_centigrates = (ADCcode*503.975f)/4096.f-273.15f;
		UARTWriteDecimal((int32_t)temp_centigrates);
		UARTWrite("\n");
	}
	else if (!strcmp(command, "help"))
	{
		// Bright blue
		UARTWrite("\033[0m\n\033[94m");
		UARTWrite("dir: Show list of files in working directory\n");
		UARTWrite("cls: Clear terminal\n");
		UARTWrite("mem: Show available memory\n");
		UARTWrite("tmp: Show device temperature\n");
		UARTWrite("del fname: Delete file\n");
		UARTWrite("cwd path: Change working directory\n");
		UARTWrite("get fname: Save binary from UART to micro sd card\n");
		UARTWrite("prc: Show process info\n");
		UARTWrite("gdb: Enter gdb server mode\n");
		UARTWrite("ver: Show version info\n");
		UARTWrite("Any other input will load a file from sd: with matching name\n");
		UARTWrite("CTRL+C terminates current program\n");
		UARTWrite("\033[0m\n");
	}
	else // Anything else defers to being a command on storage
	{
		STaskContext* tctx = GetTaskContext();
		// Temporary measure to avoid loading another executable while the first one is running
		// until we get a virtual memory device
		if (tctx->numTasks>1)
		{
			UARTWrite("Virtual memory support required to run more than one task.\n");
		}
		else
		{
			char filename[128];
			strcpy(filename, s_currentPath);
			strcat(filename, "\\");
			strcat(filename, command);
			strcat(filename, ".elf");

			// First parameter is excutable name
			s_startAddress = LoadExecutable(filename, true);
			strcpy(s_execName, filename);

			const char *param = strtok(nullptr, " ");
			// Change working directory
			if (!param)
				s_execParamCount = 1;
			else
			{
				strncpy(s_execParam, param, 64);
				s_execParamCount = 2;
			}

			// TODO: Push argc/argv onto stack

			// If we succeeded in loading the executable, the trampoline task can branch into it.
			// NOTE: Without code relocation or virtual memory, two executables will ovelap when loaded
			// even though each gets a new task slot assigned.
			// This will cause corruption of the runtime environment.
			if (s_startAddress != 0x0)
				TaskAdd(tctx, command, RunExecTask, HALF_SECOND_IN_TICKS);
		}
	}
}

int main()
{
	// Clear terminal
	LEDSetState(0xF);
	UARTWrite("\033[H\033[0m\033[2J\033[96;40mtinysys " VERSIONSTRING "\033[0m\n\n");

	// Set up internals
	LEDSetState(0xE);
	RingBufferReset();

	// Attempt to mount the FAT volume on micro sd card
	// NOTE: Loaded executables do not have to worry about this part
	LEDSetState(0xD);
	s_driveMounted = MountDrive();

	// Create task context
	LEDSetState(0xC);
	STaskContext *taskctx = CreateTaskContext();

	// With current layout, OS takes up a very small slices out of whatever is left from other tasks
	LEDSetState(0xB);
	TaskAdd(taskctx, "kernelStub", _stubTask, HALF_MILLISECOND_IN_TICKS);

	LEDSetState(0xA);

	// Start the timer and hardware interrupt handlers.
	// This is where all task switching and other interrupt handling occurs

	// NOTE: Software debugger will run in the UART interrupt handler
	// and modify task memory via commands received from gdb remote protocol.

	InstallISR();

	// Default path
	strncpy(s_currentPath, "sd:", 64);

	// Ready to start, silence LEDs
	LEDSetState(0x0);

	// Main CLI loop
	while (1)
	{
		// Echo all of the characters we can find back to the sender
		uint32_t uartData = 0;
		int execcmd = 0;
		while (RingBufferRead(&uartData, sizeof(uint32_t)))
		{
			uint8_t asciicode = (uint8_t)(uartData&0xFF);
			s_refreshConsoleOut++;
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
					execcmd++;
					// Terminate process 1 (the current executable)
					TaskExitTaskWithID(taskctx, 1, 0); // Sig:0
				}
				break;

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
					if (s_cmdLen > 127)
						s_cmdLen = 127;
				}
				break;
			}
		}

		if (execcmd)
		{
			s_refreshConsoleOut = 1;
			UARTWrite("\n");
			ExecuteCmd(s_cmdString);
			// Rewind
			s_cmdLen = 0;
			s_cmdString[0] = 0;
		}

		if (s_refreshConsoleOut)
		{
			s_refreshConsoleOut = 0;
			s_cmdString[s_cmdLen] = 0;
			// Reset current line and emit the command string
			UARTWrite("\033[2K\r");
			UARTWrite(s_currentPath);
			UARTWrite(":");
			UARTWrite(s_cmdString);
		}
	}

	return 0;
}
