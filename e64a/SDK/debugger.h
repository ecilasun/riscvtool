#pragma once

#include "task.h"

uint32_t gdb_handler(STaskContext tasks[], const uint32_t num_tasks);
uint32_t gdb_breakpoint(STaskContext tasks[]);

void AddBreakPoint(STaskContext &task, uint32_t breakaddress);
void RemoveBreakPoint(STaskContext &task, uint32_t breakaddress);
