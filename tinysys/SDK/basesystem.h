#pragma once

#include <inttypes.h>

// tinysys runs wall clock at 10MHz
#define ONE_SECOND_IN_TICKS			    	    10000000
#define TWO_HUNDRED_FIFTY_MILLISECONDS_IN_TICKS	2500000
#define TWO_HUNDRED_MILLISECONDS_IN_TICKS	    2000000
#define HUNDRED_MILLISECONDS_IN_TICKS	        1000000
#define TEN_MILLISECONDS_IN_TICKS		        100000
#define ONE_MILLISECOND_IN_TICKS		        10000
#define HALF_MILLISECOND_IN_TICKS		        5000
#define ONE_MICROSECOND_IN_TICKS		        10

#define APPMEM_START                0x00000000 // Top of RAM
#define HEAP_START_APPMEM_END       0x02000000 // 32Mbytes application space above this
#define HEAP_END_TASKMEM_START      0x0FFE0000 // 223Mbytes Heap space above this
#define TASKMEM_END_STACK_END       0x0FFEE000 // 56Kbyte space for tasks above this
#define STACK_BASE                  0x0FFEFFFC // Stack above this
#define ROMSHADOW_START             0x0FFF0000 // 4byte gap above this
#define ROMSHADOW_END_MEM_END       0x0FFFFFFF // 64Kbyte ROM shadow copy above this

// Device addresses
// Each device has a 4Kbyte region mapped to i
#define DEVICE_BASE 0x80000000
#define DEVICE_UART (DEVICE_BASE+0x0000)
#define DEVICE_LEDS (DEVICE_BASE+0x1000)
#define DEVICE_GPUC (DEVICE_BASE+0x2000)
#define DEVICE_SPIC (DEVICE_BASE+0x3000)
#define DEVICE_CSRF (DEVICE_BASE+0x4000)
#define DEVICE_XADC (DEVICE_BASE+0x5000)
#define DEVICE_DMAC (DEVICE_BASE+0x6000)

uint64_t E32ReadTime();
uint64_t E32ReadCycles();
uint64_t E32ReadRetiredInstructions();
void E32SetTimeCompare(const uint64_t future);

uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds);

void E32Sleep(uint64_t ms);
