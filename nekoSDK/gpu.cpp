#include "gpu.h"

volatile uint32_t *IO_GPUCommandFIFO = (volatile uint32_t* )0x80000000;    // GPU command FIFO
volatile uint32_t *GraphicsRAMStart = (uint32_t* )0x10000000;              // Start of Graphics RAM region (inclusive, 128KBytes)
volatile uint32_t *GraphicsRAMEnd = (uint32_t* )0x10020000;                // End of Graphics RAM region (non-inclusive)
