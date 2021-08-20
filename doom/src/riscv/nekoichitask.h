#pragma once

#include <inttypes.h>

// 1ms default task switch interval
#define DEFAULT_TIMESLICE 25000

// Currently supporting this many tasks
#define MAX_TASKS 16

struct break_point_t
{
   uint32_t address;
   uint32_t originalinstruction;
};

// Entry for a task's state
struct cpu_context
{
   const char *name;
   uint32_t reg[64]; // Task's saved registers (not storing float registers yet)
   uint32_t PC; // Task's saved program counter
   uint32_t quantum; // Run time for this task
   struct break_point_t breakpoints[8];
   uint32_t num_breakpoints;
   uint32_t ctrlc;
   uint32_t ctrlcaddress;
   uint32_t ctrlcbackup;
   uint32_t breakhit;
};
