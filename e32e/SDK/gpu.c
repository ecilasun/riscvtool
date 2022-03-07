#include "gpu.h"

volatile uint8_t *GPUFB0 = (volatile uint8_t* )0x81000000;
volatile uint8_t *GPUFB1 = (volatile uint8_t* )0x81020000;
volatile uint32_t *GPUPAL_32 = (volatile uint32_t* )0x81040000;
volatile uint32_t *GPUCTL = (volatile uint32_t* )0x81040100; // Up to 0x8104FFFF
