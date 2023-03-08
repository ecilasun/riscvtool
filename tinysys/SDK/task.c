#include "core.h"
#include "task.h"

#include <stdlib.h>

// NOTE: Tasks are core local at this point, and won't migrate between cores.

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void TaskInitSystem(struct STaskContext *_ctx)
{
	_ctx->numTasks = 0;
	_ctx->tasks = (STask*)malloc(sizeof(STask)*TASK_MAX);

	// Clean out all tasks
	for (uint32_t i=0; i<TASK_MAX; ++i)
	{
		_ctx->tasks[i].HART = 0x0; // Can't run anywhere by default
		_ctx->tasks[i].PC = 0x0;
		_ctx->tasks[i].regs[2] = 0x0;
		_ctx->tasks[i].regs[8] = 0x0;
		_ctx->tasks[i].task = (taskfunc)0x0;
		_ctx->tasks[i].ctrlc = 0;
		_ctx->tasks[i].breakhit = 0;
		_ctx->tasks[i].name[0] = 0; // No name
		_ctx->tasks[i].num_breakpoints = 0;
		for (int j=0;j<TASK_MAX_BREAKPOINTS;++j)
		{
			_ctx->tasks[i].breakpoints[j].address = 0x0;
			_ctx->tasks[i].breakpoints[j].originalinstruction = 0x0;
		}
	}

	// Set current process ID
	asm volatile("li tp, 0;");
}

int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval)
{
	int32_t prevcount = _ctx->numTasks;
	if (prevcount >= TASK_MAX)
		return 0;

	// Stop timer interrupts on this core during this operation
	//swap_csr(mie, MIP_MSIP | MIP_MEIP);

	uint32_t *stackbase = (uint32_t*)malloc(_stacksizeword*sizeof(uint32_t));
	uint32_t *stackpointer = stackbase + (_stacksizeword-1);

	// Insert the task before we increment task count
	_ctx->tasks[prevcount].PC = (uint32_t)_task;
	_ctx->tasks[prevcount].regs[2] = (uint32_t)stackpointer;	// Stack pointer
	_ctx->tasks[prevcount].regs[8] = (uint32_t)stackpointer;	// Frame pointer
	_ctx->tasks[prevcount].taskstack = stackbase;				// Stack base, to be freed when task's done
	_ctx->tasks[prevcount].stacksize = _stacksizeword;			// Stack size in WORDs

	char *np = (char*)_name;
	int idx = 0;
	while(np!=0 && idx<15)
		_ctx->tasks[prevcount].name[idx++] = *np;
	_ctx->tasks[prevcount].name[idx] = 0;

	// Increment the task count
	_ctx->numTasks = prevcount+1;

	// Resume timer interrupts on this core
	//swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return 1;
}

void TaskSwitchToNext(struct STaskContext *_ctx)
{
	// Load current process ID from TP register
	//asm volatile("mv x31, tp;");

	// TODO:
	// 1) Make sure we came here from a 'task', else bail out (for instance we might end up here from main())
	// 2) Make sure we have more than one task running, else bail out
	// 3) Save current tasks' registers and PC into its task context
	// 4) Load next tasks' registers
	// 5) Swap MEPC to next tasks' PC and mret
}
