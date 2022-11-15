#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

// 'five' has a 25MHz wall clock
#define ONE_SECOND_IN_TICKS				25000000
#define HUNDRED_MILLISECONDS_IN_TICKS	2500000
#define TEN_MILLISECONDS_IN_TICKS		250000
#define ONE_MILLISECOND_IN_TICKS		25000
#define ONE_MICROSECOND_IN_TICKS		25

uint64_t E32ReadTime();
uint64_t E32ReadCycles();
uint64_t E32ReadRetiredInstructions();
void E32SetTimeCompare(const uint64_t future);

uint32_t ClockToSec(uint64_t clk);
uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds);

void E32Sleep(uint64_t ms);

#define E32AlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))
