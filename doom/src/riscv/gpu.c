#include "gpu.h"
#include "config.h"

void GPUSetRegister(const uint8_t reg, uint32_t val) {
    *IO_GPUFIFO = GPUOPCODE(GPUSETREGISTER, 0, reg, GPU22BITIMM(val));
    *IO_GPUFIFO = GPUOPCODE(GPUSETREGISTER, reg, reg, GPU10BITIMM(val));
}

void GPUSetVideoPage(const uint8_t videoPageRegister)
{
    *IO_GPUFIFO = GPUOPCODE(GPUSETVPAGE, videoPageRegister, 0, 0);
}

void GPUWriteSystemMemory(const uint8_t countRegister, const uint8_t sysramPointerReg)
{
    *IO_GPUFIFO = GPUOPCODE(GPUSYSMEMOUT, countRegister, sysramPointerReg, 0);
}

void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked)
{
    *IO_GPUFIFO = GPUOPCODE(GPUSYSDMA, SYSRAMSourceReg, VRAMDWORDAlignedTargetReg, ((DMALengthInDWORDs&0x3FFF) | (masked ? 0x4000 : 0x0000)));
}

void GPUWaitForVsync()
{
    *IO_GPUFIFO = GPUOPCODE(GPUVSYNC, 0, 0, 0);
}

void GPUClearVRAMPage(const uint8_t clearColorRegister)
{
    *IO_GPUFIFO = GPUOPCODE(GPUCLEAR, clearColorRegister, 0, 0);
}

void GPURasterizeTriangle(const uint8_t vertex0Register, const uint8_t vertex1Register, const uint8_t vertex2Register, const uint8_t fillColorIndex)
{
    *IO_GPUFIFO = GPUOPCODE3(GPURASTERIZE, vertex0Register, vertex1Register, vertex2Register, fillColorIndex);
}

void GPUSetPaletteEntry(const uint8_t rgbRegister, const uint8_t targetPaletteEntry)
{
    // Each color component is 8 bits (8:8:8) giving a 24 bit color value
    // There are 256 slots for color palette entries
    // Color bit order in register should be: ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b)
    *IO_GPUFIFO = GPUOPCODE(GPUWRITEPALETTE, rgbRegister, 0, targetPaletteEntry);
}
