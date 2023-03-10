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

int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task)
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
	task->regs[8] = stackpointer;		// Frame pointer

	char *np = (char*)_name;
	int idx = 0;
	while(np!=0 && idx<15)
	{
		task->name[idx++] = *np;
		++np;
	}
	task->name[idx] = 0;

	// Resume timer interrupts on this core
	//write_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return 1;
}

void TaskSwitchToNext(struct STaskContext *_ctx)
{
	// Load current process ID from TP register
	int32_t currentTask = _ctx->currentTask;

	// Get current task's register stash
	uint32_t *regs = _ctx->tasks[currentTask].regs;

	// Store register back-ups in current task's structure
	asm volatile("csrr tp, 0x000; sw tp, %0;" : "=m" (regs[0]));		// PC of halted task
	asm volatile("csrr tp, 0x001; sw tp, %0;" : "=m" (regs[1]));		// ra
	asm volatile("csrr tp, 0x002; sw tp, %0;" : "=m" (regs[2]));		// sp
	asm volatile("csrr tp, 0x003; sw tp, %0;" : "=m" (regs[3]));		// gp
	asm volatile("csrr tp, 0x004; sw tp, %0;" : "=m" (regs[4]));		// tp
	asm volatile("csrr tp, 0x005; sw tp, %0;" : "=m" (regs[5]));		// t0
	asm volatile("csrr tp, 0x006; sw tp, %0;" : "=m" (regs[6]));		// t1
	asm volatile("csrr tp, 0x007; sw tp, %0;" : "=m" (regs[7]));		// t2
	asm volatile("csrr tp, 0x008; sw tp, %0;" : "=m" (regs[8]));		// s0
	asm volatile("csrr tp, 0x009; sw tp, %0;" : "=m" (regs[9]));		// s1
	asm volatile("csrr tp, 0x00A; sw tp, %0;" : "=m" (regs[10]));		// a0
	asm volatile("csrr tp, 0x00B; sw tp, %0;" : "=m" (regs[11]));		// a1
	asm volatile("csrr tp, 0x00C; sw tp, %0;" : "=m" (regs[12]));		// a2
	asm volatile("csrr tp, 0x00D; sw tp, %0;" : "=m" (regs[13]));		// a3
	asm volatile("csrr tp, 0x00E; sw tp, %0;" : "=m" (regs[14]));		// a4
	asm volatile("csrr tp, 0x00F; sw tp, %0;" : "=m" (regs[15]));		// a5
	asm volatile("csrr tp, 0x010; sw tp, %0;" : "=m" (regs[16]));		// a6
	asm volatile("csrr tp, 0x011; sw tp, %0;" : "=m" (regs[17]));		// a7
	asm volatile("csrr tp, 0x012; sw tp, %0;" : "=m" (regs[18]));		// s2
	asm volatile("csrr tp, 0x013; sw tp, %0;" : "=m" (regs[19]));		// s3
	asm volatile("csrr tp, 0x014; sw tp, %0;" : "=m" (regs[20]));		// s4
	asm volatile("csrr tp, 0x015; sw tp, %0;" : "=m" (regs[21]));		// s5
	asm volatile("csrr tp, 0x016; sw tp, %0;" : "=m" (regs[22]));		// s6
	asm volatile("csrr tp, 0x017; sw tp, %0;" : "=m" (regs[23]));		// s7
	asm volatile("csrr tp, 0x018; sw tp, %0;" : "=m" (regs[24]));		// s8
	asm volatile("csrr tp, 0x019; sw tp, %0;" : "=m" (regs[25]));		// s9
	asm volatile("csrr tp, 0x01A; sw tp, %0;" : "=m" (regs[26]));		// s10
	asm volatile("csrr tp, 0x01B; sw tp, %0;" : "=m" (regs[27]));		// s11
	asm volatile("csrr tp, 0x01C; sw tp, %0;" : "=m" (regs[28]));		// t3
	asm volatile("csrr tp, 0x01D; sw tp, %0;" : "=m" (regs[29]));		// t4
	asm volatile("csrr tp, 0x01E; sw tp, %0;" : "=m" (regs[30]));		// t5
	asm volatile("csrr tp, 0x01F; sw tp, %0;" : "=m" (regs[31]));		// t6

	// Switch to next task
	currentTask = (_ctx->numTasks <= 1) ? 0 : (currentTask+1) % _ctx->numTasks;
	_ctx->currentTask = currentTask;
	regs = _ctx->tasks[currentTask].regs;

	// Load next tasks's registers into CSR file
	asm volatile("lw tp, %0; csrw 0x000, tp;" : "=m" (regs[0]));		// PC of next task
	asm volatile("lw tp, %0; csrw 0x001, tp;" : "=m" (regs[1]));		// ra
	asm volatile("lw tp, %0; csrw 0x002, tp;" : "=m" (regs[2]));		// sp
	asm volatile("lw tp, %0; csrw 0x003, tp;" : "=m" (regs[3]));		// gp
	asm volatile("lw tp, %0; csrw 0x004, tp;" : "=m" (regs[4]));		// tp
	asm volatile("lw tp, %0; csrw 0x005, tp;" : "=m" (regs[5]));		// t0
	asm volatile("lw tp, %0; csrw 0x006, tp;" : "=m" (regs[6]));		// t1
	asm volatile("lw tp, %0; csrw 0x007, tp;" : "=m" (regs[7]));		// t2
	asm volatile("lw tp, %0; csrw 0x008, tp;" : "=m" (regs[8]));		// s0
	asm volatile("lw tp, %0; csrw 0x009, tp;" : "=m" (regs[9]));		// s1
	asm volatile("lw tp, %0; csrw 0x00A, tp;" : "=m" (regs[10]));		// a0
	asm volatile("lw tp, %0; csrw 0x00B, tp;" : "=m" (regs[11]));		// a1
	asm volatile("lw tp, %0; csrw 0x00C, tp;" : "=m" (regs[12]));		// a2
	asm volatile("lw tp, %0; csrw 0x00D, tp;" : "=m" (regs[13]));		// a3
	asm volatile("lw tp, %0; csrw 0x00E, tp;" : "=m" (regs[14]));		// a4
	asm volatile("lw tp, %0; csrw 0x00F, tp;" : "=m" (regs[15]));		// a5
	asm volatile("lw tp, %0; csrw 0x010, tp;" : "=m" (regs[16]));		// a6
	asm volatile("lw tp, %0; csrw 0x011, tp;" : "=m" (regs[17]));		// a7
	asm volatile("lw tp, %0; csrw 0x012, tp;" : "=m" (regs[18]));		// s2
	asm volatile("lw tp, %0; csrw 0x013, tp;" : "=m" (regs[19]));		// s3
	asm volatile("lw tp, %0; csrw 0x014, tp;" : "=m" (regs[20]));		// s4
	asm volatile("lw tp, %0; csrw 0x015, tp;" : "=m" (regs[21]));		// s5
	asm volatile("lw tp, %0; csrw 0x016, tp;" : "=m" (regs[22]));		// s6
	asm volatile("lw tp, %0; csrw 0x017, tp;" : "=m" (regs[23]));		// s7
	asm volatile("lw tp, %0; csrw 0x018, tp;" : "=m" (regs[24]));		// s8
	asm volatile("lw tp, %0; csrw 0x019, tp;" : "=m" (regs[25]));		// s9
	asm volatile("lw tp, %0; csrw 0x01A, tp;" : "=m" (regs[26]));		// s10
	asm volatile("lw tp, %0; csrw 0x01B, tp;" : "=m" (regs[27]));		// s11
	asm volatile("lw tp, %0; csrw 0x01C, tp;" : "=m" (regs[28]));		// t3
	asm volatile("lw tp, %0; csrw 0x01D, tp;" : "=m" (regs[29]));		// t4
	asm volatile("lw tp, %0; csrw 0x01E, tp;" : "=m" (regs[30]));		// t5
	asm volatile("lw tp, %0; csrw 0x01F, tp;" : "=m" (regs[31]));		// t6
}
