#pragma once

#include "task.h"

uint32_t gdb_handler(struct STaskContext *_ctx);
uint32_t gdb_breakpoint(struct STaskContext *_ctx);

void AddBreakPoint(struct STask *task, uint32_t breakaddress);
void RemoveBreakPoint(struct STask *task, uint32_t breakaddress);
