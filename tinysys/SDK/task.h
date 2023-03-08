#pragma once

#include <inttypes.h>

#define TASK_MAX 16
#define TASK_MAX_BREAKPOINTS 16

typedef void(*taskfunc)();

struct STaskBreakpoint
{
	uint32_t address;				// Address of replaced instruction / breakpoint
	uint32_t originalinstruction;	// Instruction we replaced with EBREAK
};

struct STask {
    taskfunc task;      // Task entry point
    uint32_t HART;      // HART affinity mask (for migration)
    uint32_t PC;        // Program counter
    uint32_t regs[64];  // Integer and float registers

	// Setup
	void *taskstack;	// Initial stack pointer, allocated by task caller
	uint32_t stacksize;	// Size of stack for this task
	char name[16];		// Name of this task

	// Debug support - this will probably move somewhere else
	uint32_t ctrlc;				// Stop on first chance
	uint32_t breakhit;			// We've stopped due to a breakpoint
	uint32_t num_breakpoints;	// Breakpoint count
	struct STaskBreakpoint breakpoints[TASK_MAX_BREAKPOINTS];	// List of breakpoints
};

struct STaskContext {
	struct STask *tasks;			// List of all the tasks
	volatile uint32_t numTasks;		// Number of tasks
};

void TaskInitSystem(struct STaskContext *_ctx);
int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval);
void TaskSwitchToNext(struct STaskContext *_ctx);