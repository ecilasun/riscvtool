#include "apu.h"

volatile uint32_t *IO_APUCommandFIFO = (volatile uint32_t* )0x80000024;    // APU command FIFO
volatile uint32_t *IO_AudioFIFO = (volatile uint32_t* )0x80000020;         // Audio FIFO
volatile uint32_t *AudioRAMStart = (uint32_t* )0x20000000;                 // Start of Audio RAM region (inclusive, 64KBytes, serves as BOOT ROM)
volatile uint32_t *AudioRAMEnd = (uint32_t* )0x20010000;                   // End of Audio RAM region (non-inclusive)
