#include "core.h"
#include "gpu.h"
#include "uart.h"

#define GPUFontSize (256*24)
uint32_t *GRAMFontStart = (uint32_t* )((uint32_t)GRAMEnd-GPUFontSize);

// 256x24 (3 rows, 32 characters on each row)
const uint8_t residentfont[] __attribute__((aligned(4))) = {
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0F, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 
0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 
0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 
0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0F, 0x0, 0x0F, 0x0, 0x0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x0F, 0x0, 0x0, 0x0, 0x0, 
};

const uint32_t VGAPalette[] __attribute__((aligned(4))) = {
0x00000000, 0x000000AA, 0x00AA0000, 0x00AA00AA,
0x0000AA00, 0x0000AAAA, 0x0055AA00, 0x00AAAAAA,
0x00555555, 0x005555FF, 0x00FF5555, 0x00FF55FF,
0x0055FF55, 0x0055FFFF, 0x00FFFF55, 0x00FFFFFF,
0x00000000, 0x00101010, 0x00202020, 0x00353535,
0x00454545, 0x00555555, 0x00656565, 0x00757575,
0x008A8A8A, 0x009A9A9A, 0x00AAAAAA, 0x00BABABA,
0x00CACACA, 0x00DFDFDF, 0x00EFEFEF, 0x00FFFFFF,
0x000000FF, 0x000041FF, 0x000082FF, 0x0000BEFF,
0x0000FFFF, 0x0000FFBE, 0x0000FF82, 0x0000FF41,
0x0000FF00, 0x0041FF00, 0x0082FF00, 0x00BEFF00,
0x00FFFF00, 0x00FFBE00, 0x00FF8200, 0x00FF4100,
0x00FF0000, 0x00FF0041, 0x00FF0082, 0x00FF00BE,
0x00FF00FF, 0x00BE00FF, 0x008200FF, 0x004100FF,
0x008282FF, 0x00829EFF, 0x0082BEFF, 0x0082DFFF,
0x0082FFFF, 0x0082FFDF, 0x0082FFBE, 0x0082FF9E,
0x0082FF82, 0x009EFF82, 0x00BEFF82, 0x00DFFF82,
0x00FFFF82, 0x00FFDF82, 0x00FFBE82, 0x00FF9E82,
0x00FF8282, 0x00FF829E, 0x00FF82BE, 0x00FF82DF,
0x00FF82FF, 0x00DF82FF, 0x00BE82FF, 0x009E82FF,
0x00BABAFF, 0x00BACAFF, 0x00BADFFF, 0x00BAEFFF,
0x00BAFFFF, 0x00BAFFEF, 0x00BAFFDF, 0x00BAFFCA,
0x00BAFFBA, 0x00CAFFBA, 0x00DFFFBA, 0x00EFFFBA,
0x00FFFFBA, 0x00FFEFBA, 0x00FFDFBA, 0x00FFCABA,
0x00FFBABA, 0x00FFBACA, 0x00FFBADF, 0x00FFBAEF,
0x00FFBAFF, 0x00EFBAFF, 0x00DFBAFF, 0x00CABAFF,
0x00000071, 0x00001C71, 0x00003971, 0x00005571,
0x00007171, 0x00007155, 0x00007139, 0x0000711C,
0x00007100, 0x001C7100, 0x00397100, 0x00557100,
0x00717100, 0x00715500, 0x00713900, 0x00711C00,
0x00710000, 0x0071001C, 0x00710039, 0x00710055,
0x00710071, 0x00550071, 0x00390071, 0x001C0071,
0x00393971, 0x00394571, 0x00395571, 0x00396171,
0x00397171, 0x00397161, 0x00397155, 0x00397145,
0x00397139, 0x00457139, 0x00557139, 0x00617139,
0x00717139, 0x00716139, 0x00715539, 0x00714539,
0x00713939, 0x00713945, 0x00713955, 0x00713961,
0x00713971, 0x00613971, 0x00553971, 0x00453971,
0x00515171, 0x00515971, 0x00516171, 0x00516971,
0x00517171, 0x00517169, 0x00517161, 0x00517159,
0x00517151, 0x00597151, 0x00617151, 0x00697151,
0x00717151, 0x00716951, 0x00716151, 0x00715951,
0x00715151, 0x00715159, 0x00715161, 0x00715169,
0x00715171, 0x00695171, 0x00615171, 0x00595171,
0x00000041, 0x00001041, 0x00002041, 0x00003141,
0x00004141, 0x00004131, 0x00004120, 0x00004110,
0x00004100, 0x00104100, 0x00204100, 0x00314100,
0x00414100, 0x00413100, 0x00412000, 0x00411000,
0x00410000, 0x00410010, 0x00410020, 0x00410031,
0x00410041, 0x00310041, 0x00200041, 0x00100041,
0x00202041, 0x00202841, 0x00203141, 0x00203941,
0x00204141, 0x00204139, 0x00204131, 0x00204128,
0x00204120, 0x00284120, 0x00314120, 0x00394120,
0x00414120, 0x00413920, 0x00413120, 0x00412820,
0x00412020, 0x00412028, 0x00412031, 0x00412039,
0x00412041, 0x00392041, 0x00312041, 0x00282041,
0x002D2D41, 0x002D3141, 0x002D3541, 0x002D3D41,
0x002D4141, 0x002D413D, 0x002D4135, 0x002D4131,
0x002D412D, 0x0031412D, 0x0035412D, 0x003D412D,
0x0041412D, 0x00413D2D, 0x0041352D, 0x0041312D,
0x00412D2D, 0x00412D31, 0x00412D35, 0x00412D3D,
0x00412D41, 0x003D2D41, 0x00352D41, 0x00312D41,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000 };

void UploadFont()
{
    __builtin_memcpy(GRAMFontStart, residentfont, GPUFontSize);
}

int main()
{
    UARTWrite("GPU functionality test\n\n");

    UARTWrite(" -resetting G-RAM contents to zero\n");
    __builtin_memset((void*)GRAMStart, 0x00, sizeof(uint32_t)*0xFFFF);

    UARTWrite(" -resetting P-RAM contents to zero\n");
    __builtin_memset((void*)PRAMStart, 0x00, sizeof(uint32_t)*0xFFFF);

    UARTWrite(" -uploading font\n");
    UploadFont();

    // Create a command package
    UARTWrite(" -creating command buffer\n");
    GPUCommandPackage cmd;

    // Set up default VGA palette data somewhere in G-RAM,
    // then change one of the colors
    for (uint32_t i=0;i<256;++i)
        GRAMStart[i] = VGAPalette[i];
    GRAMStart[0x0F] = 0x00ACD943; // This is the text color for font bitmap

    // Reset write cursor and prepare package
    GPUInitializeCommandPackage(&cmd);

    // Write the prologue, since this is the first program and goes at 0x0000
    GPUWritePrologue(&cmd);

    // Choose page#0 for writes (and page#1 for display output)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE));           // vpage zero

    // Set palette color 0x0F to a tint of yellow from white as a test (font uses this color)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_DMA, G_R0, G_R0, G_DMAGRAMTOPALETTE, 0x1000)); // dma.p zero, zero, 256 // copy palette from G-RAM top (0) starting at palette memory top (0)

    // Byte increment for VRAM writes
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, 0x0000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, 0x0004));   // setregi r15, 0x00000004

    // Write 4 pixels to VRAM start
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, 0xFFFF));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, 0xFFFF));   // setregi r4, 0xFFFFFFFF
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R5, G_HIGHBITS, G_R5, 0x8000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R5, G_LOWBITS, G_R5, 0x0000));   // setregi r5, 0x80000000
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_STORE, G_R4, G_R5, 0x0, G_WORD));          // store.w r5, r4

    // Set next 4 pixels to another color
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, 0x0405));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, 0x0607));  // setregi r4, 0x04050607

    // Write 4 more pixels at next word address
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_ALU, G_R5, G_R15, G_R5, G_ADD));          // add.w r5, r15, r5
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_STORE, G_R4, G_R5, 0x0, G_WORD));         // store.w r5, r4

    // DMA test (copy all font words from G-RAM to V-RAM starting at current addres)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, HIHALF((uint32_t)GRAMFontStart)));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, LOHALF((uint32_t)GRAMFontStart)));  // setregi r4, (uint32_t)GRAMFontStart
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_DMA, G_R4, G_R5, 0x0, GPUFontSize/4));                             // dma r4, r5, GPUFontSize/4

    // Choose page#1 for writes (and page#0 for display output)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R8, G_HIGHBITS, G_R8, 0x0000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R8, G_LOWBITS, G_R8, 0x0001));  // setregi r8, 0x00000001
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_MISC, G_R8, 0x0, 0x0, G_VPAGE));          // vpage r8

    // Write epilogue that will put the GPU into halt state once the program is complete
    GPUWriteEpilogue(&cmd);

    // Finish the package by updating the submit word count
    GPUCloseCommandPackage(&cmd);

    // DEBUG: Dump command package
    /*for (uint32_t i=0;i<cmd.m_wordcount;++i)
    {
        UARTWriteHex(cmd.m_commands[i]);
        UARTWrite("\n");
    }*/

    UARTWrite(" -submitting command list\n");
    GPUClearMailbox();
    GPUSubmitCommands(&cmd);

    UARTWrite(" -validating commands\n");
    int has_errors = GPUValidateCommands(&cmd);
    if (has_errors == -1)
        UARTWrite(" -ERROR: command package errors detected\n");
    else
    {
        UARTWrite(" -kicking GPU work\n");
        GPUKick();

        UARTWrite(" -waiting for GPU write to mailbox\n");
        int retval = GPUWaitMailbox();

        if (retval == 0)
            UARTWrite(" -SUCCESS: GPU wrote to mailbox\n");
        else
            UARTWrite(" -ERROR: GPU mailbox write timed out\n");
    }

    return 0;
}
