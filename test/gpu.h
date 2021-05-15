
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

// Plots a single byte into given VRAM address (To be deprecated)
#define GPUWRITEVRAM 0x2

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

inline void GPUSetRegister(const uint8_t reg, uint32_t val) {
    GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, reg, GPU22BITIMM(val));
    GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, reg, reg, GPU10BITIMM(val));
}

inline void GPUSetVideoPage(const uint8_t videoPageRegister)
{
    GPUFIFO[0] = GPUOPCODE(GPUSETVPAGE, videoPageRegister, 0, 0);
}

inline void GPUWriteSystemMemory(const uint8_t countRegister, const uint8_t statePointerReg)
{
    GPUFIFO[0] = GPUOPCODE(GPUSYSMEMOUT, countRegister, statePointerReg, 0);
}

inline void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked)
{
    GPUFIFO[0] = GPUOPCODE(GPUSYSDMA, SYSRAMSourceReg, VRAMDWORDAlignedTargetReg, ((DMALengthInDWORDs&0x3FFF) | (masked ? 0x4000 : 0x0000)));
}

inline void GPUWaitForVsync()
{
    GPUFIFO[0] = GPUOPCODE(GPUVSYNC, 0, 0, 0);
}

inline void GPUClearVRAMPage(const uint8_t clearColorRegister)
{
    GPUFIFO[0] = GPUOPCODE(GPUCLEAR, clearColorRegister, 0, 0);
}

inline void GPURasterizeTriangle(const uint8_t vertex0Register, const uint8_t vertex1Register, const uint8_t vertex2Register, const uint8_t fillColor)
{
    GPUFIFO[0] = GPUOPCODE3(GPURASTERIZE, vertex0Register, vertex1Register, vertex2Register, fillColor);
}

inline void GPUWriteVideoMemory(const uint8_t writeValueRegister, const uint8_t writeMask, const uint32_t DWORDAlignedVideoMemoryAddress)
{
    GPUFIFO[0] = GPUOPCODE2(GPUWRITEVRAM, writeValueRegister, 0, writeMask, (DWORDAlignedVideoMemoryAddress&0x3FFF));
}
