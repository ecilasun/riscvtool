#include "core.h"
#include "task.h"

#include <stdlib.h>

const char *dummyname = "n/a";

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void InitTasks(struct STaskContext *_ctx)
{
	// Clean out all tasks
	for (uint32_t i=0; i<MAX_TASKS; ++i)
	{
		_ctx[i].HART = 0x0; // Can't run anywhere by default
		_ctx[i].interval = TEN_MILLISECONDS_IN_TICKS;
		_ctx[i].PC = 0x0;
		_ctx[i].regs[2] = 0x0;
		_ctx[i].regs[8] = 0x0;
		_ctx[i].task = (taskfunc)0x0;
		_ctx[i].ctrlc = 0;
		_ctx[i].breakhit = 0;
		_ctx[i].name = dummyname;
		_ctx[i].num_breakpoints = 0;
		for (int j=0;j<TASK_MAX_BREAKPOINTS;++j)
		{
			_ctx[i].breakpoints[j].address = 0x0;
			_ctx[i].breakpoints[j].originalinstruction = 0x0;
		}
	}
}

// NOTE: Can only add a task to this core's context
// HINT: Use MAILBOX and HARTIRQ to send across a task to be added to a remote core
int AddTask(struct STaskContext *_ctx, volatile uint32_t *taskcount, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval)
{
	uint32_t prevcount = (*taskcount);
	if (prevcount >= MAX_TASKS)
		return 0;

	uint32_t *stackbase = (uint32_t*)malloc(_stacksizeword*sizeof(uint32_t));
	uint32_t *stackpointer = stackbase + (_stacksizeword-1);

	// Insert the task before we increment task count
	_ctx[prevcount].PC = (uint32_t)_task;
	_ctx[prevcount].regs[2] = (uint32_t)stackpointer;	// Stack pointer
	_ctx[prevcount].regs[8] = (uint32_t)stackpointer;	// Frame pointer
	_ctx[prevcount].interval = _interval;				// Execution interval
	_ctx[prevcount].taskstack = stackbase;				// Stack base, to be freed when task's done
	_ctx[prevcount].stacksize = _stacksizeword;			// Stack size in WORDs

	// Stop timer interrupts on this core during this operation
	swap_csr(mie, MIP_MSIP | MIP_MEIP);
	// Increment the task count
	(*taskcount) = prevcount+1;
	// Resume timer interrupts on this core
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return 1;
}
