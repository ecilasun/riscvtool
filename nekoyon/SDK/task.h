#pragma once

#include <inttypes.h>

// 1ms default task switch interval
#define DEFAULT_TIMESLICE 10000 // 10ms

// NOTE:
// 1000 // 1.ms - 1000usec
// 100 // 0.1ms - 100usec
// 10 // 0.01ms - 10usec
// 1 // 0.001ms - 1usec
// 1ns == 0.0000001ms therefore not enough resolution for nanosleep
// We can only do microsecond sleeps with the current counter speed
// For nanosecond sleeps, need to increase timer precision in H/W

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
   uint32_t reg[32]{0}; // Task's saved registers (not storing float registers yet)
   uint32_t PC{0}; // Task's saved program counter
   uint32_t quantum{DEFAULT_TIMESLICE}; // Run time for this task
   break_point_t breakpoints[8];
   uint32_t num_breakpoints{0};
   uint32_t ctrlc{0};
   uint32_t ctrlcaddress{0};
   uint32_t ctrlcbackup{0};
   uint32_t breakhit{0};
};
