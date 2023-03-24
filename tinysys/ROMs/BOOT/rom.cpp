// Boot ROM

#include "rvcrt0.h"
#include "rombase.h"

#include <string.h>

#define VERSIONSTRING "v0.987"

static char s_cmdString[128];
static char s_currentPath[64];
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

void uget(const char *savename)
{
	// TODO: Load file from remote over UART
	if (!savename)
		UARTWrite("usage: uget targetfilename\r\n");
	else
		UARTWrite("TODO: Add task to receive files\r\n");
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
			UARTWrite(" Mbytes\r\n");
		}
		else
		{
			UARTWriteDecimal(inkbytes);
			UARTWrite(" Kbytes\r\n");
		}
	}
	else if (!strcmp(command, "ps"))
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
			UARTWrite("\r\n");
		}
	}
	else if (!strcmp(command, "del"))
	{
		const char *path = strtok(nullptr, " ");
		// TODO: delete a file
		if (!path)
			UARTWrite("usage: del fname\r\n");
		else
			UARTWrite("TODO: Delete given file\r\n");
	}
	else if (!strcmp(command, "cwd"))
	{
		const char *path = strtok(nullptr, " ");
		// Change working directory
		if (!path)
			UARTWrite("usage: cwd path\r\n");
		else
			strncpy(s_currentPath, path, 128);
	}
	else if (!strcmp(command, "uget"))
	{
		const char *savename = strtok(nullptr, " ");
		uget(savename);
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
		// Bright blue
		UARTWrite("\033[0m\r\n\033[94m");
		UARTWrite("dir: Show list of files in working directory\r\n");
		UARTWrite("cls: Clear terminal\r\n");
		UARTWrite("mem: Show available memory\r\n");
		UARTWrite("del fname: Delete file\r\n");
		UARTWrite("cwd path: Change working directory\r\n");
		UARTWrite("uget fname: Save binary from UART to micro sd card\r\n");
		UARTWrite("ps: Show process list\r\n");
		UARTWrite("gdb: Enter gdb server mode\r\n");
		UARTWrite("ver: Show version info\r\n");
		UARTWrite("Any other input will load a file from sd: with matching name\r\n");
		UARTWrite("CTRL+C terminates current program\r\n");
		UARTWrite("\033[0m\r\n");
	}
	else // Anything else defers to being a command on storage
	{
		char filename[128];
		strcpy(filename, s_currentPath);
		strcat(filename, "\\");
		strcat(filename, command);
		strcat(filename, ".elf");

		// First parameter is excutable name
		s_startAddress = LoadExecutable(filename, true);

		// If we succeeded in loading the executable, the trampoline task can branch into it.
		// NOTE: Without code relocation or virtual memory, two executables will ovelap when loaded
		// even though each gets a new task slot assigned.
		// This will cause corruption of the runtime environment.
		if (s_startAddress != 0x0)
			TaskAdd(GetTaskContext(), command, RunExecTask, HUNDRED_MILLISECONDS_IN_TICKS);
	}
}

int main()
{
	// Clear terminal
	UARTWrite("\033[H\033[0m\033[2Jtinysys " VERSIONSTRING "\r\nType 'help' for CLI usage info\r\n\r\n");

	strncpy(s_currentPath, "sd:", 128);

	// Set up internals
	RingBufferReset();

	// Attempt to mount the FAT volume on micro sd card
	// NOTE: Loaded executables do not have to worry about this part
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

	int stringchanged = 1;

	while (1) {
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
					execcmd++;
					// Terminate process 1 (the current executable)
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
			UARTWrite("\033[2K\r");
			UARTWrite(s_currentPath);
			UARTWrite(":");
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
