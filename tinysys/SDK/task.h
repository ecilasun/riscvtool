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
	uint32_t HART;      // HART affinity mask (for migration)
	uint32_t regs[32];  // Integer registers - NOTE: register zero here is actually the PC

	// Debug support - this will probably move somewhere else
	char name[16];				// Name of this task
	uint32_t ctrlc;				// Stop on first chance
	uint32_t breakhit;			// We've stopped due to a breakpoint
	uint32_t num_breakpoints;	// Breakpoint count
	struct STaskBreakpoint breakpoints[TASK_MAX_BREAKPOINTS];	// List of breakpoints
};

struct STaskContext {
	struct STask *tasks;			// List of all the tasks
	volatile int32_t currentTask;	// Current task index
	volatile int32_t numTasks;		// Number of tasks
};

void TaskInitSystem(struct STaskContext *_ctx);
int TaskAdd(struct STaskContext *_ctx, const char *_name, taskfunc _task/*, uint32_t _stacksizeword*/);
void TaskSwitchToNext(struct STaskContext *_ctx);