#pragma once

#include <inttypes.h>

// Utility macros
#define MAKERGB8BITCOLOR(_r, _g, _b) (((_b>>6)<<6) | ((_r>>5)<<3) | ((_g>>5)))

// GPU command macros
#define GPU22BITIMM(_immed_) (_immed_&0x003FFFFF)
#define GPU10BITIMM(_immed_) ((_immed_&0xFFC00000)>>22)
#define GPUOPCODE(_cmd_, _rs_, _rd_, _imm_) (_imm_<<10)|(_rd_<<7)|(_rs_<<4)|(_cmd_)
#define GPUOPCODE2(_cmd_, _rs_, _rd_,_mask_, _imm_) (_imm_<<14) | (_mask_<<10) | (_rd_<<7) | (_rs_<<4) | (_cmd_);
#define GPUOPCODE3(_cmd_, _rs1_, _rs2_, _rs3_, _imm_) (_imm_<<13)|(_rs3_<<10)|(_rs2_<<7)|(_rs1_<<4)|(_cmd_)

// GPU opcodes (only lower 3 bits out of 4 used)

// Stalls the GPU until next vsync
#define GPUVSYNC 0x0

// Set register lower 22 bits when source register is zero, otherwise sets uppper 10 bits
#define GPUSETREGISTER 0x1

// Sets an entry in the palette to given RGB color in source register
#define GPUWRITEPALETTE 0x2

// Clears the whole VRAM to color in source register (at x12 speed of continuous DWORD writes)
#define GPUCLEAR 0x3

// Copies DWORD aligned SYSRAM address in rs to DWORD aligned VRAM address in rd, by given DWORDs
#define GPUSYSDMA 0x4

// Rasterize primitive
#define GPURASTERIZE 0x5

// Write DWORD in rs onro SYSRAM address in rd, effectively signalling CPU from GPU side
#define GPUSYSMEMOUT 0x6

// Set VRAM write page
#define GPUSETVPAGE 0x7

void GPUSetRegister(const uint8_t reg, uint32_t val);
void GPUSetVideoPage(const uint8_t videoPageRegister);
void GPUWriteSystemMemory(const uint8_t countRegister, const uint8_t sysramPointerReg);
void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked);
void GPUWaitForVsync();
void GPUClearVRAMPage(const uint8_t clearColorRegister);
void GPURasterizeTriangle(const uint8_t vertex0Register, const uint8_t vertex1Register, const uint8_t vertex2Register, const uint8_t fillColorIndex);
void GPUSetPaletteEntry(const uint8_t rgbRegister, const uint8_t targetPaletteEntry);
