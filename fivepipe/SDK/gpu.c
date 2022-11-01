// TODO: #include <assert.h>
#include "gpu.h"

// NOTE: Writes to this FIFO address will set the current video scanout address
// For the current version, only bits [31:18] of the VRAM address are considered valid
volatile uint32_t *GPUFIFO = (volatile uint32_t* )0x80000030;

// Dedicated video memory region, not normally seen by programs
// This is a cached memory region, so the CPU has to flush the cache for the
// output to become visible to the GPU
volatile uint32_t *VRAM = (volatile uint32_t* )0x20100000;

// Same as VRAM but accessed as bytes
volatile uint8_t *VRAMBYTES = (volatile uint8_t* )0x20100000;

void GPUSetVPage(uint32_t _scanOutAddress64ByteAligned)
{
    // TODO: assert((_scanOutAddress64ByteAligned&0x3F) == 0);

    *GPUFIFO = GPUCMD_SETVPAGE;
    *GPUFIFO = _scanOutAddress64ByteAligned;
}

void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _rgba24)
{
    *GPUFIFO = GPUCMD_SETPAL;
    *GPUFIFO = (_paletteIndex<<24) | (_rgba24&0x00FFFFFFFF);
}