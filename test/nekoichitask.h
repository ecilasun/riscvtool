#pragma once

#include <inttypes.h>

// 10ms default task switch interval
#define DEFAULT_TIMESLICE 100000

// Currently supporting this many tasks
#define MAX_TASKS 16

struct break_point_t
{
   uint32_t address{0};
   uint32_t originalinstruction{0};
};

// Entry for a task's state
struct cpu_context
{
   uint32_t reg[64]{0}; // Task's saved registers (not storing float registers yet)
   uint32_t PC{0}; // Task's saved program counter
   uint32_t quantum{DEFAULT_TIMESLICE}; // Run time for this task
   break_point_t breakpoints[8];
   uint32_t num_breakpoints{0};
   uint32_t ctrlc{0};
   uint32_t ctrlcaddress{0};
   uint32_t ctrlcbackup{0};
   uint32_t breakhit{0};
};
