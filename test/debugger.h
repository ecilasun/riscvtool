#pragma once

#include "nekoichitask.h"

uint32_t gdb_handler(cpu_context tasks[], uint32_t in_breakpoint = 0);

void AddBreakPoint(cpu_context &task, uint32_t breakaddress);
void RemoveBreakPoint(cpu_context &task, uint32_t breakaddress);
