#include "core.h"
#include "task.h"
#include "leds.h"
#include "uart.h"

#include <stdlib.h>

// NOTE: Tasks are core local at this point, and won't migrate between cores.

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void TaskInitSystem(struct STaskContext *_ctx)
{
	_ctx->currentTask = 0;
	_ctx->numTasks = 0;
	_ctx->debugMode = 0;

	// Clean out all tasks
	for (uint32_t i=0; i<TASK_MAX; ++i)
	{
		_ctx->tasks[i].HART = 0x1; // Default affinity mask is HART#0
		_ctx->tasks[i].regs[0] = 0x0;	// Entry point
		_ctx->tasks[i].regs[2] = 0x0;	// Stack pointer
		_ctx->tasks[i].regs[4] = i;		// Task ID
		_ctx->tasks[i].regs[8] = 0x0;	// Frame pointer
		_ctx->tasks[i].ctrlc = 0;
		_ctx->tasks[i].breakhit = 0;
		_ctx->tasks[i].name[0] = 0; // No name
		_ctx->tasks[i].num_breakpoints = 0;
		for (int j=0; j<TASK_MAX_BREAKPOINTS; ++j)
		{
			_ctx->tasks[i].breakpoints[j].address = 0x0;
			_ctx->tasks[i].breakpoints[j].originalinstruction = 0x0;
		}
	}
}

int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task, const uint32_t _runLength)
{
	int32_t prevcount = _ctx->numTasks;
	if (prevcount >= TASK_MAX)
		return 0;

	// Stop timer interrupts on this core during this operation
	//write_csr(mie, MIP_MSIP | MIP_MEIP);

	++_ctx->numTasks;

	// Task stacks start at 0x0FFEE000
	const uint32_t stacksizeword = 1024;
	uint32_t stackpointer = 0x0FFEE000 - (prevcount*stacksizeword);

	// Insert the task before we increment task count
	struct STask *task = &(_ctx->tasks[prevcount]);
	task->regs[0] = (uint32_t)_task;	// PC
	task->regs[2] = stackpointer;		// Stack pointer
	task->regs[4] = prevcount;			// Thread pointer - taskID
	task->regs[8] = stackpointer;		// Frame pointer
	task->runLength = _runLength;		// Time slice dedicated to this task

	char *np = (char*)_name;
	int idx = 0;
	while(np!=0 && idx<15)
	{
		task->name[idx++] = *np;
		++np;
	}
	task->name[idx] = 0;

	// We assume running state as soon as we start
	task->state = TS_RUNNING;

	// Resume timer interrupts on this core
	//write_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return 1;
}

void TaskExitCurrentTask(struct STaskContext *_ctx)
{
	int32_t currentTask = read_csr(0x004); // TP
	if (currentTask == 0) // Can not exit taskid 0
		return;
	_ctx->tasks[currentTask].state = TS_TERMINATING;
	write_csr(0x00A, 0); // Return a0 = 0x0
}

void TaskExitTaskWithID(struct STaskContext *_ctx, uint32_t _taskid, uint32_t _signal)
{
	if (_taskid == 0) // Can not exit taskid 0
		return;
	_ctx->tasks[_taskid].state = TS_TERMINATING;
	write_csr(0x00A, _signal); // Return a0 = _signal
}

uint32_t TaskSwitchToNext(struct STaskContext *_ctx)
{
	// Load current process ID from TP register
	int32_t currentTask = _ctx->currentTask;

	// Get current task's register stash
	uint32_t *regs = _ctx->tasks[currentTask].regs;

	// Store register back-ups in current task's structure
	regs[0] = read_csr(0x000);	// PC of halted task
	regs[1] = read_csr(0x001);	// ra
	regs[2] = read_csr(0x002);	// sp
	regs[3] = read_csr(0x003);	// gp
	regs[4] = read_csr(0x004);	// tp
	regs[5] = read_csr(0x005);	// t0
	regs[6] = read_csr(0x006);	// t1
	regs[7] = read_csr(0x007);	// t2
	regs[8] = read_csr(0x008);	// s0
	regs[9] = read_csr(0x009);	// s1
	regs[10] = read_csr(0x00A);	// a0
	regs[11] = read_csr(0x00B);	// a1
	regs[12] = read_csr(0x00C);	// a2
	regs[13] = read_csr(0x00D);	// a3
	regs[14] = read_csr(0x00E);	// a4
	regs[15] = read_csr(0x00F);	// a5
	regs[16] = read_csr(0x010);	// a6
	regs[17] = read_csr(0x011);	// a7
	regs[18] = read_csr(0x012);	// s2
	regs[19] = read_csr(0x013);	// s3
	regs[20] = read_csr(0x014);	// s4
	regs[21] = read_csr(0x015);	// s5
	regs[22] = read_csr(0x016);	// s6
	regs[23] = read_csr(0x017);	// s7
	regs[24] = read_csr(0x018);	// s8
	regs[25] = read_csr(0x019);	// s9
	regs[26] = read_csr(0x01A);	// s10
	regs[27] = read_csr(0x01B);	// s11
	regs[28] = read_csr(0x01C);	// t3
	regs[29] = read_csr(0x01D);	// t4
	regs[30] = read_csr(0x01E);	// t5
	regs[31] = read_csr(0x01F);	// t6

	// Break
	if (_ctx->tasks[currentTask].ctrlc == 1)
	{
		_ctx->tasks[currentTask].ctrlc = 2;
		_ctx->tasks[currentTask].ctrlcaddress = regs[0];				// Save PC of the instruction on which we stopped
		_ctx->tasks[currentTask].ctrlcbackup = *(uint32_t*)(regs[0]);	// Save the instruction at this address
		*(uint32_t*)(regs[0]) = 0x00100073;								// Replace it with EBREAK instruction
		CFLUSH_D_L1;													// Make sure the write makes it to RAM
		FENCE_I;														// Make sure I$ is flushed so it can see this write
	}

	// Resume
	if (_ctx->tasks[currentTask].ctrlc == 8)
	{
		_ctx->tasks[currentTask].breakhit = 0;
		_ctx->tasks[currentTask].ctrlc = 0;
		*(uint32_t*)(_ctx->tasks[currentTask].ctrlcaddress) = _ctx->tasks[currentTask].ctrlcbackup;	// Restore old instruction
		CFLUSH_D_L1;																				// Make sure the write makes it to RAM
		FENCE_I;																					// Make sure I$ is flushed so it can see this write
	}

	// Switch to next task
	currentTask = (_ctx->numTasks <= 1) ? 0 : ((currentTask+1) % _ctx->numTasks);
	_ctx->currentTask = currentTask;

	// Terminate task
	// NOTE: Task #0 cannot be terminated
	if (_ctx->tasks[currentTask].state == TS_TERMINATING && currentTask != 0)
	{
		/*UARTWrite("\033[31m\033[40mTask '");
		UARTWrite(_ctx->tasks[currentTask].name);
		UARTWrite("' exited with return code 0x");
		UARTWriteHex(regs[10]);
		UARTWrite("\033[0m\n");*/
		// Mark as 'terminated'
		_ctx->tasks[currentTask].state = TS_TERMINATED;
		// Replace with task at end of list, if we're not the end of list
		if (currentTask != _ctx->numTasks-1)
			__builtin_memcpy(&_ctx->tasks[currentTask], &_ctx->tasks[_ctx->numTasks-1], sizeof(struct STask));
		// One less task to run
		--_ctx->numTasks;
		// Rewind back to OS Idle task (always guaranteed to be alive)
		_ctx->currentTask = 0;
		currentTask = 0;
	}

	regs = _ctx->tasks[currentTask].regs;

	// Load next tasks's registers into CSR file
	write_csr(0x000, regs[0]);	// PC of next task
	write_csr(0x001, regs[1]);	// ra
	write_csr(0x002, regs[2]);	// sp
	write_csr(0x003, regs[3]);	// gp
	write_csr(0x004, regs[4]);	// tp
	write_csr(0x005, regs[5]);	// t0
	write_csr(0x006, regs[6]);	// t1
	write_csr(0x007, regs[7]);	// t2
	write_csr(0x008, regs[8]);	// s0
	write_csr(0x009, regs[9]);	// s1
	write_csr(0x00A, regs[10]);	// a0
	write_csr(0x00B, regs[11]);	// a1
	write_csr(0x00C, regs[12]);	// a2
	write_csr(0x00D, regs[13]);	// a3
	write_csr(0x00E, regs[14]);	// a4
	write_csr(0x00F, regs[15]);	// a5
	write_csr(0x010, regs[16]);	// a6
	write_csr(0x011, regs[17]);	// a7
	write_csr(0x012, regs[18]);	// s2
	write_csr(0x013, regs[19]);	// s3
	write_csr(0x014, regs[20]);	// s4
	write_csr(0x015, regs[21]);	// s5
	write_csr(0x016, regs[22]);	// s6
	write_csr(0x017, regs[23]);	// s7
	write_csr(0x018, regs[24]);	// s8
	write_csr(0x019, regs[25]);	// s9
	write_csr(0x01A, regs[26]);	// s10
	write_csr(0x01B, regs[27]);	// s11
	write_csr(0x01C, regs[28]);	// t3
	write_csr(0x01D, regs[29]);	// t4
	write_csr(0x01E, regs[30]);	// t5
	write_csr(0x01F, regs[31]);	// t6

	return _ctx->tasks[currentTask].runLength;
}
