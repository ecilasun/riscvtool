#pragma once

#include "nekoichitask.h"

uint32_t gdb_handler(cpu_context tasks[], const uint32_t num_tasks);
uint32_t gdb_breakpoint(cpu_context tasks[]);

void AddBreakPoint(cpu_context &task, uint32_t breakaddress);
void RemoveBreakPoint(cpu_context &task, uint32_t breakaddress);
