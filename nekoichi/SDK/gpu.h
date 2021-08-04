
#include <inttypes.h>

extern volatile uint32_t *IO_GPUCommandFIFO;
extern volatile uint32_t *GraphicsRAMStart;
extern volatile uint32_t *GraphicsRAMEnd;
extern volatile uint32_t *GraphicsFontStart;

// Utility macros
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g)<<16) | ((_r)<<8) | (_b))

// GPU command FIFO macros
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

// Copies DWORD aligned GRAM address in rs to DWORD aligned VRAM address in rd, by given DWORDs
#define GPUSYSDMA 0x4

// Rasterize primitive
#define GPURASTERIZE 0x5

// Write DWORD in rs onto GRAM address in rd, effectively signalling CPU from GPU side
#define GPUGMEMOUT 0x6

// Set VRAM write page
#define GPUSETVPAGE 0x7

inline void GPUSetRegister(const uint8_t reg, uint32_t val) {
    *IO_GPUCommandFIFO = GPUOPCODE(GPUSETREGISTER, 0, reg, GPU22BITIMM(val));
    *IO_GPUCommandFIFO = GPUOPCODE(GPUSETREGISTER, reg, reg, GPU10BITIMM(val));
}

inline void GPUSetVideoPage(const uint8_t videoPageRegister)
{
    *IO_GPUCommandFIFO = GPUOPCODE(GPUSETVPAGE, videoPageRegister, 0, 0);
}

inline void GPUWriteToGraphicsMemory(const uint8_t countRegister, const uint8_t sysramPointerReg)
{
    *IO_GPUCommandFIFO = GPUOPCODE(GPUGMEMOUT, countRegister, sysramPointerReg, 0);
}

inline void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked)
{
    *IO_GPUCommandFIFO = GPUOPCODE(GPUSYSDMA, SYSRAMSourceReg, VRAMDWORDAlignedTargetReg, ((DMALengthInDWORDs&0x7FFF) | (masked ? 0x8000 : 0x0000)));
}

inline void GPUWaitForVsync()
{
    *IO_GPUCommandFIFO = GPUOPCODE(GPUVSYNC, 0, 0, 0);
}

inline void GPUClearVRAMPage(const uint8_t clearColorRegister)
{
    *IO_GPUCommandFIFO = GPUOPCODE(GPUCLEAR, clearColorRegister, 0, 0);
}

inline void GPURasterizeTriangle(const uint8_t vertex0Register, const uint8_t vertex1Register, const uint8_t vertex2Register, const uint8_t fillColorIndex)
{
    *IO_GPUCommandFIFO = GPUOPCODE3(GPURASTERIZE, vertex0Register, vertex1Register, vertex2Register, fillColorIndex);
}

inline void GPUSetPaletteEntry(const uint8_t rgbRegister, const uint8_t targetPaletteEntry)
{
    // Each color component is 8 bits (8:8:8) giving a 24 bit color value
    // There are 256 slots for color palette entries
    // Color bit order in register should be: ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b)
    *IO_GPUCommandFIFO = GPUOPCODE(GPUWRITEPALETTE, rgbRegister, 0, targetPaletteEntry);
}


// Utilities

void ClearScreen(const uint8_t color);
void InitFont();
void PrintDMA(const int ox, const int oy, const char *message, bool masked=true);
void PrintDMA(const int col, const int row, const int maxlen, const char *message, bool masked=true);
void PrintDMAHex(const int ox, const int oy, const uint32_t i);
uint32_t PrintDMADecimal(const int ox, const int oy, const int i, bool masked=true);
