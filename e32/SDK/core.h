#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

#define ONE_SECOND_IN_TICKS 10'000'000

uint64_t E32ReadTime();
uint32_t ClockToMs(uint64_t clk);
void E32SetTimeCompare(const uint64_t future);
