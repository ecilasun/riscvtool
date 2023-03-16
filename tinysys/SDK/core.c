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

uint8_t* brk_point = __heap_start;

int core_brk(uint32_t addr)
{
	if (brk_point>__heap_end || brk_point<__heap_start)
		return -1;

	brk_point = (uint8_t*)addr;
	return 0;
}

int core_memavail()
{
	return (__heap_end-brk_point);
}

#endif
