#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

#define ONE_SECOND_IN_TICKS 10'000'000

uint64_t E32ReadTime();
void E32SetTimeCompare(const uint64_t future);
