#include "core.h"
#include "task.h"
#include "leds.h"

#include <stdlib.h>

// NOTE: Tasks are core local at this point, and won't migrate between cores.

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void TaskInitSystem(struct STaskContext *_ctx)
{
	_ctx->currentTask = 0;
	_ctx->numTasks = 0;
	_ctx->tasks = (struct STask*)malloc(sizeof(struct STask)*TASK_MAX);

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
		for (int j=0;j<TASK_MAX_BREAKPOINTS;++j)
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

	// Task stacks start at 0xFFEE000
	const uint32_t stacksizeword = 1024;
	uint32_t stackpointer = 0xFFEE000 - (prevcount*stacksizeword);

	// Insert the task before we increment task count
	struct STask *task = &_ctx->tasks[prevcount];
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
	LEDSetState((currentTask%2==0) ? 0xFFFFFFFF : 0x00000000);

	// Get current task's register stash
	uint32_t *regs = _ctx->tasks[currentTask].regs;

	// Store register back-ups in current task's structure
	asm volatile("csrr tp, 0xFE0; sw tp, %0;" : "=m" (regs[1]));		// ra
	asm volatile("csrr tp, 0xFE1; sw tp, %0;" : "=m" (regs[2]));		// sp
	asm volatile("csrr tp, 0xFE2; sw tp, %0;" : "=m" (regs[3]));		// gp
	asm volatile("csrr tp, 0xFE3; sw tp, %0;" : "=m" (regs[4]));		// tp
	asm volatile("csrr tp, 0xFE4; sw tp, %0;" : "=m" (regs[5]));		// t0
	asm volatile("csrr tp, 0xFE5; sw tp, %0;" : "=m" (regs[6]));		// t1
	asm volatile("csrr tp, 0xFE6; sw tp, %0;" : "=m" (regs[7]));		// t2
	asm volatile("csrr tp, 0xFE7; sw tp, %0;" : "=m" (regs[8]));		// s0
	asm volatile("csrr tp, 0xFE8; sw tp, %0;" : "=m" (regs[9]));		// s1
	asm volatile("csrr tp, 0xFE9; sw tp, %0;" : "=m" (regs[10]));		// a0
	asm volatile("csrr tp, 0xFEA; sw tp, %0;" : "=m" (regs[11]));		// a1
	asm volatile("csrr tp, 0xFEB; sw tp, %0;" : "=m" (regs[12]));		// a2
	asm volatile("csrr tp, 0xFEC; sw tp, %0;" : "=m" (regs[13]));		// a3
	asm volatile("csrr tp, 0xFED; sw tp, %0;" : "=m" (regs[14]));		// a4
	asm volatile("csrr tp, 0xFEE; sw tp, %0;" : "=m" (regs[15]));		// a5
	asm volatile("csrr tp, 0xFEF; sw tp, %0;" : "=m" (regs[16]));		// a6
	asm volatile("csrr tp, 0xFF0; sw tp, %0;" : "=m" (regs[17]));		// a7
	asm volatile("csrr tp, 0xFF1; sw tp, %0;" : "=m" (regs[18]));		// s2
	asm volatile("csrr tp, 0xFF2; sw tp, %0;" : "=m" (regs[19]));		// s3
	asm volatile("csrr tp, 0xFF3; sw tp, %0;" : "=m" (regs[20]));		// s4
	asm volatile("csrr tp, 0xFF4; sw tp, %0;" : "=m" (regs[21]));		// s5
	asm volatile("csrr tp, 0xFF5; sw tp, %0;" : "=m" (regs[22]));		// s6
	asm volatile("csrr tp, 0xFF6; sw tp, %0;" : "=m" (regs[23]));		// s7
	asm volatile("csrr tp, 0xFF7; sw tp, %0;" : "=m" (regs[24]));		// s8
	asm volatile("csrr tp, 0xFF8; sw tp, %0;" : "=m" (regs[25]));		// s9
	asm volatile("csrr tp, 0xFF9; sw tp, %0;" : "=m" (regs[26]));		// s10
	asm volatile("csrr tp, 0xFFA; sw tp, %0;" : "=m" (regs[27]));		// s11
	asm volatile("csrr tp, 0xFFB; sw tp, %0;" : "=m" (regs[28]));		// t3
	asm volatile("csrr tp, 0xFFC; sw tp, %0;" : "=m" (regs[29]));		// t4
	asm volatile("csrr tp, 0xFFD; sw tp, %0;" : "=m" (regs[30]));		// t5
	asm volatile("csrr tp, 0xFFE; sw tp, %0;" : "=m" (regs[31]));		// t6
	asm volatile("csrr tp, 0xFFF; sw tp, %0;" : "=m" (regs[0]));		// PC of halted task

	// Switch to next task
	currentTask = (_ctx->numTasks <= 1) ? 0 : (currentTask+1) % _ctx->numTasks;
	_ctx->currentTask = currentTask;
	regs = _ctx->tasks[currentTask].regs;

	// Load next tasks's registers into CSR file
	asm volatile("lw tp, %0; csrw 0xFE0, tp;" : "=m" (regs[1]));		// ra
	asm volatile("lw tp, %0; csrw 0xFE1, tp;" : "=m" (regs[2]));		// sp
	asm volatile("lw tp, %0; csrw 0xFE2, tp;" : "=m" (regs[3]));		// gp
	asm volatile("lw tp, %0; csrw 0xFE3, tp;" : "=m" (regs[4]));		// tp
	asm volatile("lw tp, %0; csrw 0xFE4, tp;" : "=m" (regs[5]));		// t0
	asm volatile("lw tp, %0; csrw 0xFE5, tp;" : "=m" (regs[6]));		// t1
	asm volatile("lw tp, %0; csrw 0xFE6, tp;" : "=m" (regs[7]));		// t2
	asm volatile("lw tp, %0; csrw 0xFE7, tp;" : "=m" (regs[8]));		// s0
	asm volatile("lw tp, %0; csrw 0xFE8, tp;" : "=m" (regs[9]));		// s1
	asm volatile("lw tp, %0; csrw 0xFE9, tp;" : "=m" (regs[10]));		// a0
	asm volatile("lw tp, %0; csrw 0xFEA, tp;" : "=m" (regs[11]));		// a1
	asm volatile("lw tp, %0; csrw 0xFEB, tp;" : "=m" (regs[12]));		// a2
	asm volatile("lw tp, %0; csrw 0xFEC, tp;" : "=m" (regs[13]));		// a3
	asm volatile("lw tp, %0; csrw 0xFED, tp;" : "=m" (regs[14]));		// a4
	asm volatile("lw tp, %0; csrw 0xFEE, tp;" : "=m" (regs[15]));		// a5
	asm volatile("lw tp, %0; csrw 0xFEF, tp;" : "=m" (regs[16]));		// a6
	asm volatile("lw tp, %0; csrw 0xFF0, tp;" : "=m" (regs[17]));		// a7
	asm volatile("lw tp, %0; csrw 0xFF1, tp;" : "=m" (regs[18]));		// s2
	asm volatile("lw tp, %0; csrw 0xFF2, tp;" : "=m" (regs[19]));		// s3
	asm volatile("lw tp, %0; csrw 0xFF3, tp;" : "=m" (regs[20]));		// s4
	asm volatile("lw tp, %0; csrw 0xFF4, tp;" : "=m" (regs[21]));		// s5
	asm volatile("lw tp, %0; csrw 0xFF5, tp;" : "=m" (regs[22]));		// s6
	asm volatile("lw tp, %0; csrw 0xFF6, tp;" : "=m" (regs[23]));		// s7
	asm volatile("lw tp, %0; csrw 0xFF7, tp;" : "=m" (regs[24]));		// s8
	asm volatile("lw tp, %0; csrw 0xFF8, tp;" : "=m" (regs[25]));		// s9
	asm volatile("lw tp, %0; csrw 0xFF9, tp;" : "=m" (regs[26]));		// s10
	asm volatile("lw tp, %0; csrw 0xFFA, tp;" : "=m" (regs[27]));		// s11
	asm volatile("lw tp, %0; csrw 0xFFB, tp;" : "=m" (regs[28]));		// t3
	asm volatile("lw tp, %0; csrw 0xFFC, tp;" : "=m" (regs[29]));		// t4
	asm volatile("lw tp, %0; csrw 0xFFD, tp;" : "=m" (regs[30]));		// t5
	asm volatile("lw tp, %0; csrw 0xFFE, tp;" : "=m" (regs[31]));		// t6
	asm volatile("lw tp, %0; csrw 0xFFF, tp;" : "=m" (regs[0]));		// PC of next task
}
