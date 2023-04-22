#include "audio.h"

volatile uint32_t *IO_AUDIOOUT = (volatile uint32_t* )0x80008000;

void APUSetRate(uint32_t repeatSampleCount)
{
    *IO_AUDIOOUT = APUCMD_SETRATE;
    *IO_AUDIOOUT = repeatSampleCount;
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

void APUSwapBuffers(uint32_t syncPoint)
{
    *IO_AUDIOOUT = APUCMD_SWAP;
    *IO_AUDIOOUT = syncPoint;
}
