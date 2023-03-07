#pragma once

#include <inttypes.h>

#define MAX_TASKS 16
#define TASK_MAX_BREAKPOINTS 16

typedef void(*taskfunc)();

struct STaskBreakpoint
{
	uint32_t address;				// Address of replaced instruction / breakpoint
	uint32_t originalinstruction;	// Instruction we replaced with EBREAK
};

struct STaskContext {
    taskfunc task;      // Task entry point
    uint32_t HART;      // HART affinity mask (for migration)
    uint32_t PC;        // Program counter
    uint32_t interval;  // Execution interval
    uint32_t regs[64];  // Integer and float registers

	// Debug support
	uint32_t ctrlc;							// Stop on first chance
	uint32_t breakhit;						// We've stopped due to a breakpoint
	uint32_t num_breakpoints;				// Breakpoint count
	struct STaskBreakpoint breakpoints[TASK_MAX_BREAKPOINTS];	// List of breakpoints

	// Setup
	void *taskstack;	// Initial stack pointer, allocated by task caller
	uint32_t stacksize;	// Size of stack for this task
	const char *name;	// Name of this task
};

void TaskInitSystem(struct STaskContext *_ctx);
int TaskAdd(struct STaskContext *_ctx, volatile uint32_t *taskcount, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval);
void TaskSwitchToTask(volatile uint32_t *currentTask, volatile uint32_t *taskcount, struct STaskContext *_ctx);