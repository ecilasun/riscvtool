#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

// syscall helpers
int core_brk(uint32_t brkptr);

#define E32AlignUp(_x_, _align_) ((_x_ + (_align_ - 1)) & (~(_align_ - 1)))

// Flush data cache to memory
#define CFLUSH_D_L1 asm volatile( ".word 0xFC000073;" )
// Discard data cache contents
#define CDISCARD_D_L1 asm volatile( ".word 0xFC200073;" )
// Invalidate instruction cache
#define FENCE_I asm volatile( "fence.i;" )
