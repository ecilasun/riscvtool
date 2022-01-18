#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

#define ONE_SECOND_IN_TICKS 10000000
#define ONE_MILLISECOND_IN_TICKS 10000
#define ONE_MICROSECOND_IN_TICKS 10

uint64_t E32ReadTime();
uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void E32SetTimeCompare(const uint64_t future);
