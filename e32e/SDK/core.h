#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

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
void E32SetTimeCompare(const uint64_t future);
uint64_t E32ReadRetiredInstructions();

extern volatile uint32_t *HARTMAILBOX;
extern volatile uint8_t *HARTIRQ;

// User defined timer interrupt service routine signature
typedef void (*t_timerISR)();
