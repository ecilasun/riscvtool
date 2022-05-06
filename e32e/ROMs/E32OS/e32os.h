
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "elf.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "buttons.h"
#include "ps2.h"
#include "ringbuffer.h"
#include "gpu.h"
//#include "debugger.h"

//asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1 (writeback D$)
//asm volatile( ".word 0xFC200073;" ); // CDISCARD.D.L1 (invalidate D$)
//asm volatile("fence.i;"); // FENCE.I (invalidate I$)
