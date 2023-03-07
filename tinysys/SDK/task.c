#include "core.h"
#include "task.h"

#include <stdlib.h>

const char *g_dummyTaskName = "n/a";

// NOTE: Tasks are core local at this point, and won't migrate between cores.

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void TaskInitSystem(struct STaskContext *_ctx)
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
		_ctx[i].name = g_dummyTaskName;
		_ctx[i].num_breakpoints = 0;
		for (int j=0;j<TASK_MAX_BREAKPOINTS;++j)
		{
			_ctx[i].breakpoints[j].address = 0x0;
			_ctx[i].breakpoints[j].originalinstruction = 0x0;
		}
	}
}

int TaskAdd(struct STaskContext *_ctx, volatile uint32_t *taskcount, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval)
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
	//swap_csr(mie, MIP_MSIP | MIP_MEIP);
	// Increment the task count
	(*taskcount) = prevcount+1;
	// Resume timer interrupts on this core
	//swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return 1;
}

void TaskSwitchToTask(volatile uint32_t *currentTask, volatile uint32_t *taskcount, struct STaskContext *_ctx)
{
	// TODO:
	// 1) Make sure we came here from a 'task', else bail out (for instance we might end up here from main())
	// 2) Make sure we have more than one task running, else bail out
	// 3) Save current tasks' registers and PC into its task context
	// 4) Load next tasks' registers
	// 5) Swap MEPC to next tasks' PC and mret
}

/*
// Save a task's registers (all except TP and GP)
#define SAVEREGS(_tsk) {\
	asm volatile("lw tp, 144(sp); sw tp, %0;" : "=m" (_tsk.regs[1]) ); \
	asm volatile("lw tp, 140(sp); sw tp, %0;" : "=m" (_tsk.regs[2]) ); \
	asm volatile("lw tp, 136(sp); sw tp, %0;" : "=m" (_tsk.regs[5]) ); \
	asm volatile("lw tp, 132(sp); sw tp, %0;" : "=m" (_tsk.regs[6]) ); \
	asm volatile("lw tp, 128(sp); sw tp, %0;" : "=m" (_tsk.regs[7]) ); \
	asm volatile("lw tp, 124(sp); sw tp, %0;" : "=m" (_tsk.regs[10]) ); \
	asm volatile("lw tp, 120(sp); sw tp, %0;" : "=m" (_tsk.regs[11]) ); \
	asm volatile("lw tp, 116(sp); sw tp, %0;" : "=m" (_tsk.regs[12]) ); \
	asm volatile("lw tp, 112(sp); sw tp, %0;" : "=m" (_tsk.regs[13]) ); \
	asm volatile("lw tp, 108(sp); sw tp, %0;" : "=m" (_tsk.regs[14]) ); \
	asm volatile("lw tp, 104(sp); sw tp, %0;" : "=m" (_tsk.regs[15]) ); \
	asm volatile("lw tp, 100(sp); sw tp, %0;" : "=m" (_tsk.regs[16]) ); \
	asm volatile("lw tp,  96(sp); sw tp, %0;" : "=m" (_tsk.regs[17]) ); \
	asm volatile("lw tp,  92(sp); sw tp, %0;" : "=m" (_tsk.regs[28]) ); \
	asm volatile("lw tp,  88(sp); sw tp, %0;" : "=m" (_tsk.regs[29]) ); \
	asm volatile("lw tp,  84(sp); sw tp, %0;" : "=m" (_tsk.regs[30]) ); \
	asm volatile("lw tp,  80(sp); sw tp, %0;" : "=m" (_tsk.regs[31]) ); \
	asm volatile("lw tp,  72(sp); sw tp, %0;" : "=m" (_tsk.regs[8]) ); \
	asm volatile("lw tp,  68(sp); sw tp, %0;" : "=m" (_tsk.regs[9]) ); \
	asm volatile("lw tp,  64(sp); sw tp, %0;" : "=m" (_tsk.regs[18]) ); \
	asm volatile("lw tp,  60(sp); sw tp, %0;" : "=m" (_tsk.regs[19]) ); \
	asm volatile("lw tp,  56(sp); sw tp, %0;" : "=m" (_tsk.regs[20]) ); \
	asm volatile("lw tp,  52(sp); sw tp, %0;" : "=m" (_tsk.regs[21]) ); \
	asm volatile("lw tp,  48(sp); sw tp, %0;" : "=m" (_tsk.regs[22]) ); \
	asm volatile("lw tp,  44(sp); sw tp, %0;" : "=m" (_tsk.regs[23]) ); \
	asm volatile("lw tp,  40(sp); sw tp, %0;" : "=m" (_tsk.regs[24]) ); \
	asm volatile("lw tp,  36(sp); sw tp, %0;" : "=m" (_tsk.regs[25]) ); \
	asm volatile("lw tp,  32(sp); sw tp, %0;" : "=m" (_tsk.regs[26]) ); \
	asm volatile("lw tp,  28(sp); sw tp, %0;" : "=m" (_tsk.regs[27]) ); \
	asm volatile("csrr tp, mepc;  sw tp, %0;" : "=m" (_tsk.PC) ); }

// Restore a task's registers (all except TP and GP)
#define RESTOREREGS(_tsk) {\
	asm volatile("lw tp, %0; sw tp, 144(sp);" : : "m" (_tsk.regs[1]) ); \
	asm volatile("lw tp, %0; sw tp, 140(sp);" : : "m" (_tsk.regs[2]) ); \
	asm volatile("lw tp, %0; sw tp, 136(sp);" : : "m" (_tsk.regs[5]) ); \
	asm volatile("lw tp, %0; sw tp, 132(sp);" : : "m" (_tsk.regs[6]) ); \
	asm volatile("lw tp, %0; sw tp, 128(sp);" : : "m" (_tsk.regs[7]) ); \
	asm volatile("lw tp, %0; sw tp, 124(sp);" : : "m" (_tsk.regs[10]) ); \
	asm volatile("lw tp, %0; sw tp, 120(sp);" : : "m" (_tsk.regs[11]) ); \
	asm volatile("lw tp, %0; sw tp, 116(sp);" : : "m" (_tsk.regs[12]) ); \
	asm volatile("lw tp, %0; sw tp, 112(sp);" : : "m" (_tsk.regs[13]) ); \
	asm volatile("lw tp, %0; sw tp, 108(sp);" : : "m" (_tsk.regs[14]) ); \
	asm volatile("lw tp, %0; sw tp, 104(sp);" : : "m" (_tsk.regs[15]) ); \
	asm volatile("lw tp, %0; sw tp, 100(sp);" : : "m" (_tsk.regs[16]) ); \
	asm volatile("lw tp, %0; sw tp,  96(sp);" : : "m" (_tsk.regs[17]) ); \
	asm volatile("lw tp, %0; sw tp,  92(sp);" : : "m" (_tsk.regs[28]) ); \
	asm volatile("lw tp, %0; sw tp,  88(sp);" : : "m" (_tsk.regs[29]) ); \
	asm volatile("lw tp, %0; sw tp,  84(sp);" : : "m" (_tsk.regs[30]) ); \
	asm volatile("lw tp, %0; sw tp,  80(sp);" : : "m" (_tsk.regs[31]) ); \
	asm volatile("lw tp, %0; sw tp,  72(sp);" : : "m" (_tsk.regs[8]) ); \
	asm volatile("lw tp, %0; sw tp,  68(sp);" : : "m" (_tsk.regs[9]) ); \
	asm volatile("lw tp, %0; sw tp,  64(sp);" : : "m" (_tsk.regs[18]) ); \
	asm volatile("lw tp, %0; sw tp,  60(sp);" : : "m" (_tsk.regs[19]) ); \
	asm volatile("lw tp, %0; sw tp,  56(sp);" : : "m" (_tsk.regs[20]) ); \
	asm volatile("lw tp, %0; sw tp,  52(sp);" : : "m" (_tsk.regs[21]) ); \
	asm volatile("lw tp, %0; sw tp,  48(sp);" : : "m" (_tsk.regs[22]) ); \
	asm volatile("lw tp, %0; sw tp,  44(sp);" : : "m" (_tsk.regs[23]) ); \
	asm volatile("lw tp, %0; sw tp,  40(sp);" : : "m" (_tsk.regs[24]) ); \
	asm volatile("lw tp, %0; sw tp,  36(sp);" : : "m" (_tsk.regs[25]) ); \
	asm volatile("lw tp, %0; sw tp,  32(sp);" : : "m" (_tsk.regs[26]) ); \
	asm volatile("lw tp, %0; sw tp,  28(sp);" : : "m" (_tsk.regs[27]) ); \
	asm volatile("lw tp, %0; csrrw zero, mepc, tp;" : : "m" (_tsk.PC) ); }
*/