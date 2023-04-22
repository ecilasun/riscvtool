#pragma once

#include <inttypes.h>

#define APUCMD_BUFFERSIZE  0x00000000
#define APUCMD_START       0x00000001
#define APUCMD_STOP        0x00000002
#define APUCMD_SWAP        0x00000003
#define APUCMD_SETRATE     0x00000004

extern volatile uint32_t *IO_AUDIOOUT;

void APUSetBufferSize(uint32_t audioBufferSize);
void APUStartDMA(uint32_t audioBufferAddress16byteAligned);
void APUStop();
void APUSwapBuffers();
void APUSetRate(uint32_t repeatSampleCount);

inline uint32_t APUFrame() { return *IO_AUDIOOUT; }
