#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

// tinysys runs wall clock at 10MHz
#define ONE_SECOND_IN_TICKS				10000000
#define HUNDRED_MILLISECONDS_IN_TICKS	1000000
#define TEN_MILLISECONDS_IN_TICKS		100000
#define ONE_MILLISECOND_IN_TICKS		10000
#define ONE_MICROSECOND_IN_TICKS		10

uint64_t E32ReadTime();
uint64_t E32ReadCycles();
uint64_t E32ReadRetiredInstructions();
void E32SetTimeCompare(const uint64_t future);

uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds);

#define E32AlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))

// Flush data cache to memory
#define CFLUSH_D_L1 asm volatile( ".word 0xFC000073;" )
// Discard data cache contents
#define CDISCARD_D_L1 asm volatile( ".word 0xFC200073;" )
// Invalidate instruction cache
#define FENCE_I asm volatile( "fence.i" )
