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
	uint32_t *stackpointer = stackbase + (_stacksizeword-1); // End of allocated memory

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
/*
	// Load current process ID from TP register
	uint32_t currenttask;
	asm volatile("mv a5, tp; sw a5, %0;" : "=m" (currenttask));

	// Get current task's register stash
	uint32_t *regs = _ctx->tasks[currenttask].regs;

	// Store register back-ups in current task's structure
	asm volatile("csrr a5, 0xFE0; sw a5, %0;" : "=m" (regs[1]));		// ra
	asm volatile("csrr a5, 0xFE1; sw a5, %0;" : "=m" (regs[2]));		// sp
	asm volatile("csrr a5, 0xFE2; sw a5, %0;" : "=m" (regs[3]));		// gp
	asm volatile("csrr a5, 0xFE3; sw a5, %0;" : "=m" (regs[4]));		// tp
	asm volatile("csrr a5, 0xFE4; sw a5, %0;" : "=m" (regs[5]));		// t0
	asm volatile("csrr a5, 0xFE5; sw a5, %0;" : "=m" (regs[6]));		// t1
	asm volatile("csrr a5, 0xFE6; sw a5, %0;" : "=m" (regs[7]));		// t2
	asm volatile("csrr a5, 0xFE7; sw a5, %0;" : "=m" (regs[8]));		// s0
	asm volatile("csrr a5, 0xFE8; sw a5, %0;" : "=m" (regs[9]));		// s1
	asm volatile("csrr a5, 0xFE9; sw a5, %0;" : "=m" (regs[10]));		// a0
	asm volatile("csrr a5, 0xFEA; sw a5, %0;" : "=m" (regs[11]));		// a1
	asm volatile("csrr a5, 0xFEB; sw a5, %0;" : "=m" (regs[12]));		// a2
	asm volatile("csrr a5, 0xFEC; sw a5, %0;" : "=m" (regs[13]));		// a3
	asm volatile("csrr a5, 0xFED; sw a5, %0;" : "=m" (regs[14]));		// a4
	asm volatile("csrr a5, 0xFEE; sw a5, %0;" : "=m" (regs[15]));		// a5
	asm volatile("csrr a5, 0xFEF; sw a5, %0;" : "=m" (regs[16]));		// a6
	asm volatile("csrr a5, 0xFF0; sw a5, %0;" : "=m" (regs[17]));		// a7
	asm volatile("csrr a5, 0xFF1; sw a5, %0;" : "=m" (regs[18]));		// s2
	asm volatile("csrr a5, 0xFF2; sw a5, %0;" : "=m" (regs[19]));		// s3
	asm volatile("csrr a5, 0xFF3; sw a5, %0;" : "=m" (regs[20]));		// s4
	asm volatile("csrr a5, 0xFF4; sw a5, %0;" : "=m" (regs[21]));		// s5
	asm volatile("csrr a5, 0xFF5; sw a5, %0;" : "=m" (regs[22]));		// s6
	asm volatile("csrr a5, 0xFF6; sw a5, %0;" : "=m" (regs[23]));		// s7
	asm volatile("csrr a5, 0xFF7; sw a5, %0;" : "=m" (regs[24]));		// s8
	asm volatile("csrr a5, 0xFF8; sw a5, %0;" : "=m" (regs[25]));		// s9
	asm volatile("csrr a5, 0xFF9; sw a5, %0;" : "=m" (regs[26]));		// s10
	asm volatile("csrr a5, 0xFFA; sw a5, %0;" : "=m" (regs[27]));		// s11
	asm volatile("csrr a5, 0xFFB; sw a5, %0;" : "=m" (regs[28]));		// t3
	asm volatile("csrr a5, 0xFFC; sw a5, %0;" : "=m" (regs[29]));		// t4
	asm volatile("csrr a5, 0xFFD; sw a5, %0;" : "=m" (regs[30]));		// t5
	asm volatile("csrr a5, 0xFFE; sw a5, %0;" : "=m" (regs[31]));		// t6
	asm volatile("csrr a5, 0xFFF; sw a5, %0;" : "=m" (_ctx->tasks[currenttask].PC));

	// Switch to next task
	currenttask = (_ctx->numTasks <= 1) ? 0 : (currenttask+1) % _ctx->numTasks;
	regs = _ctx->tasks[currenttask].regs;

	// Load next tasks's registers into CSR file
	asm volatile("lw a5, %0; csrrw zero, 0xFE0, a5;" : "=m" (regs[1]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE1, a5;" : "=m" (regs[2]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE2, a5;" : "=m" (regs[3]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE3, a5;" : "=m" (regs[4]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE4, a5;" : "=m" (regs[5]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE5, a5;" : "=m" (regs[6]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE6, a5;" : "=m" (regs[7]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE7, a5;" : "=m" (regs[8]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE8, a5;" : "=m" (regs[9]));
	asm volatile("lw a5, %0; csrrw zero, 0xFE9, a5;" : "=m" (regs[10]));
	asm volatile("lw a5, %0; csrrw zero, 0xFEA, a5;" : "=m" (regs[11]));
	asm volatile("lw a5, %0; csrrw zero, 0xFEB, a5;" : "=m" (regs[12]));
	asm volatile("lw a5, %0; csrrw zero, 0xFEC, a5;" : "=m" (regs[13]));
	asm volatile("lw a5, %0; csrrw zero, 0xFED, a5;" : "=m" (regs[14]));
	asm volatile("lw a5, %0; csrrw zero, 0xFEE, a5;" : "=m" (regs[15]));
	asm volatile("lw a5, %0; csrrw zero, 0xFEF, a5;" : "=m" (regs[16]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF0, a5;" : "=m" (regs[17]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF1, a5;" : "=m" (regs[18]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF2, a5;" : "=m" (regs[19]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF3, a5;" : "=m" (regs[20]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF4, a5;" : "=m" (regs[21]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF5, a5;" : "=m" (regs[22]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF6, a5;" : "=m" (regs[23]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF7, a5;" : "=m" (regs[24]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF8, a5;" : "=m" (regs[25]));
	asm volatile("lw a5, %0; csrrw zero, 0xFF9, a5;" : "=m" (regs[26]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFA, a5;" : "=m" (regs[27]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFB, a5;" : "=m" (regs[28]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFC, a5;" : "=m" (regs[29]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFD, a5;" : "=m" (regs[30]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFE, a5;" : "=m" (regs[31]));
	asm volatile("lw a5, %0; csrrw zero, 0xFFF, a5;" : "=m" (_ctx->tasks[currenttask].PC)); // Return address of next task
*/
}
