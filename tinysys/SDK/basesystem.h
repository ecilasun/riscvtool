#pragma once

#include <inttypes.h>

// tinysys runs wall clock at 10MHz
#define ONE_SECOND_IN_TICKS			    	    10000000
#define HALF_SECOND_IN_TICKS					5000000
#define TWO_HUNDRED_FIFTY_MILLISECONDS_IN_TICKS	2500000
#define TWO_HUNDRED_MILLISECONDS_IN_TICKS	    2000000
#define HUNDRED_MILLISECONDS_IN_TICKS	        1000000
#define TEN_MILLISECONDS_IN_TICKS		        100000
#define TWO_MILLISECONDS_IN_TICKS		        20000
#define ONE_MILLISECOND_IN_TICKS		        10000
#define HALF_MILLISECOND_IN_TICKS		        5000
#define QUARTER_MILLISECOND_IN_TICKS		    2500
#define ONE_MICROSECOND_IN_TICKS		        10

// Physical address map (when there's no MMU)
#define APPMEM_START                   0x00000000 // Top of RAM
#define HEAP_START_APPMEM_END          0x02000000 // Executable space above this (32MBytes)
#define HEAP_END_CONSOLEMEM_START      0x0FF00000 // Heap space above this (223MBytes)
#define CONSOLEMEM_END_KERNEL_VRAM_TOP 0x0FF10000 // Console text+attrib+scratch memory above this (64KBytes)
#define VRAM_END_TASKMEM_START         0x0FF30000 // Kernel VRAM above this (128KBytes)
#define TASKMEM_END_STACK_END          0x0FFD0000 // Tasks stack space above this
#define STACK_BASE                     0x0FFDFFFC // Kernel stack above this
#define ROMSHADOW_START                0x0FFE0000 // Gap above this (4Bytes)
#define ROMSHADOW_END_MEM_END          0x0FFFFFFF // ROM shadow copy above this (128KBytes)

// Device address base
#define DEVICE_BASE 0x80000000

// Each device has 4 Kbytes of continous, uncached memory region mapped to it
#define DEVICE_UART (DEVICE_BASE+0x0000)
#define DEVICE_LEDS (DEVICE_BASE+0x1000)
#define DEVICE_GPUC (DEVICE_BASE+0x2000)
#define DEVICE_SPIC (DEVICE_BASE+0x3000)
#define DEVICE_CSRF (DEVICE_BASE+0x4000)
#define DEVICE_XADC (DEVICE_BASE+0x5000)
#define DEVICE_DMAC (DEVICE_BASE+0x6000)
#define DEVICE_USBC (DEVICE_BASE+0x7000)
#define DEVICE_APUC (DEVICE_BASE+0x8000)
#define DEVICE_OPL2 (DEVICE_BASE+0x9000)
// NOTE: Add more devices after this point

uint64_t E32ReadTime();
uint64_t E32ReadCycles();
uint64_t E32ReadRetiredInstructions();
void E32SetTimeCompare(const uint64_t future);

uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds);

void E32Sleep(uint64_t ticks);
