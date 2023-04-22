#include "apu.h"
#include "core.h"
#include <stdlib.h>

volatile uint32_t *IO_AUDIOOUT = (volatile uint32_t* )0x80008000;

// APU buffers are allocated aligned to 64byte boundaries
uint8_t *APUAllocateBuffer(const uint32_t _size)
{
   void *buffer = (uint8_t*)malloc(_size + 64);
   return (uint8_t*)E32AlignUp((uint32_t)buffer, 64);
}

void APUSetBufferSize(uint32_t audioBufferSize)
{
    *IO_AUDIOOUT = APUCMD_BUFFERSIZE;
    *IO_AUDIOOUT = audioBufferSize;
}

void APUStartDMA(uint32_t audioBufferAddress16byteAligned)
{
    *IO_AUDIOOUT = APUCMD_START;
    *IO_AUDIOOUT = audioBufferAddress16byteAligned;
}

void APUStop()
{
    *IO_AUDIOOUT = APUCMD_STOP;
}

void APUSwapBuffers()
{
    *IO_AUDIOOUT = APUCMD_SWAP;
}

void APUSetRate(uint32_t repeatSampleCount)
{
    *IO_AUDIOOUT = APUCMD_SETRATE;
    *IO_AUDIOOUT = repeatSampleCount;
}
