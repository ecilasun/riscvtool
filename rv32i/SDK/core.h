#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

#define REQ_CheckAlive			0x00000000
#define REQ_InstallTISR			0x00000001
#define REQ_StartExecutable		0x00000002

#define ONE_SECOND_IN_TICKS				10000000
#define HUNDRED_MILLISECONDS_IN_TICKS	1000000
#define TEN_MILLISECONDS_IN_TICKS		100000
#define ONE_MILLISECOND_IN_TICKS		10000
#define ONE_MICROSECOND_IN_TICKS		10

// For E32E architecture, HART count is 8
#define NUM_HARTS 8

uint64_t E32ReadTime();
uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void ClockMsToHMS(uint32_t ms, uint32_t *hours, uint32_t *minutes, uint32_t *seconds);
void E32SetTimeCompare(const uint64_t future);
uint64_t E32ReadRetiredInstructions();

extern volatile uint32_t *HARTMAILBOX;
extern volatile uint8_t *HARTIRQ;

// User defined timer interrupt service routine signature, set HARTMAILBOX[hartID*HARTPARAMCOUNT+0+NUM_HARTS] to nonzero to stay alive
typedef void (*t_timerISR)(const uint32_t hartID);
void InstallTimerISR(const uint32_t hartID, t_timerISR tisr, const uint32_t interval);

// Number of parameters per hart, stored at offset NUM_HARTS
// To access a parameter N, use: NUM_HARTS+hartid*HARTPARAMCOUNT+N where N<HARTPARAMCOUNT
#define HARTPARAMCOUNT 4
#define NUMHARTWORDS 512

// Flush data cache to memory
#define CFLUSH_D_L1 asm volatile( ".word 0xFC000073;" )
// Discard data cache contents
#define CDISCARD_D_L1 asm volatile( ".word 0xFC200073;" )
// Invalidate instruction cache
#define FENCE_I asm volatile( "fence.i" )
