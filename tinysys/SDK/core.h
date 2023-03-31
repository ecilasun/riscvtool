#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

#if defined(BUILDING_ROM)
// syscall handlers for ROM
uint32_t core_brk(uint32_t brkptr);
uint32_t core_memavail();
#endif

#define E32AlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))

// Flush data cache to memory
#define CFLUSH_D_L1 asm volatile( ".word 0xFC000073;" )
// Discard data cache contents
#define CDISCARD_D_L1 asm volatile( ".word 0xFC200073;" )
// Invalidate instruction cache
#define FENCE_I asm volatile( "fence.i;" )
