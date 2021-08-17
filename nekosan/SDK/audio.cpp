#include "audio.h"

volatile uint32_t *IO_AudioFIFO = (volatile uint32_t* )0x80000018;         // Audio FIFO

volatile uint32_t *ARAMStart = (uint32_t* )0x20000000;                     // Start of ARAM region (inclusive, 32Kbytes)
volatile uint32_t *ARAMEnd = (uint32_t* )0x20008000;                       // End of ARAM region (non-inclusive)
