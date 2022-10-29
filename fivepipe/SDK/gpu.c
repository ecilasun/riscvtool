#include "leds.h"

// NOTE: Writes to this FIFO address will set the current video scanout address
// For the current version, only bits [31:18] of the VRAM address are considered valid
volatile uint32_t *GPUFIFO = (volatile uint32_t* )0x80000030;

// Dedicated video memory region, not normally seen by programs
// This is a cached memory region, so the CPU has to flush the cache for the
// output to become visible to the GPU
volatile uint32_t *VRAM = (volatile uint32_t* )0x00100000;
