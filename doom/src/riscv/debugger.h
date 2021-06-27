#pragma once

#include "nekoichitask.h"

uint32_t gdb_handler(struct cpu_context tasks[], const uint32_t num_tasks);
uint32_t gdb_breakpoint(struct cpu_context tasks[]);

void AddBreakPoint(struct cpu_context *task, uint32_t breakaddress);
void RemoveBreakPoint(struct cpu_context *task, uint32_t breakaddress);
