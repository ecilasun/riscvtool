#include <stdlib.h>
#include <errno.h>
#include "core.h"
#include "uart.h"

#include <sys/stat.h>

#ifndef DISABLE_FILESYSTEM
#include "fat32/ff.h"
#endif

#if defined(BUILDING_ROM)

extern uint8_t* __heap_start;
extern uint8_t* __heap_end;

int core_brk(uint32_t addr)
{
	__heap_end = (uint8_t*)addr;
	return 0;
}
#endif
