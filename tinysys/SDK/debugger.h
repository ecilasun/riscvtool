#pragma once

#include "task.h"

uint32_t gdb_handler(struct STaskContext tasks[], const uint32_t num_tasks);
uint32_t gdb_breakpoint(struct STaskContext tasks[]);

void AddBreakPoint(struct STaskContext *task, uint32_t breakaddress);
void RemoveBreakPoint(struct STaskContext *task, uint32_t breakaddress);
