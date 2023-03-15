#pragma once

#include "basesystem.h"
#include "task.h"

STaskContext *CreateTaskContext();
STaskContext *GetTaskContext();
void TaskDebugMode(uint32_t _mode);
void InstallISR();
