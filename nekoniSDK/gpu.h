
#include <inttypes.h>

extern volatile uint32_t *IO_GPUCommandFIFO;
extern volatile uint32_t *GraphicsRAMStart;
extern volatile uint32_t *GraphicsRAMEnd;
extern volatile uint32_t *GraphicsFontStart;

// Utility macros
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g)<<16) | ((_r)<<8) | (_b))

// GPU command FIFO macros
#define GPU8BITIMMHI(_immed_) ((_immed_&0xFF000000)>>24)
#define GPU24BITIMM(_immed_) (_immed_&0x00FFFFFF)
#define GPU20BITIMM(_immed_) (_immed_&0x000FFFFF)
#define GPUOPCODE20(_cmd_, _rd_, _rs_, _imm_) (_imm_<<12)|(_rs_<<8)|(_rd_<<4)|(_cmd_)
#define GPUOPCODE24(_cmd_, _rd_, _imm_) (_imm_<<8)|(_rd_<<4)|(_cmd_)

// GPU opcodes (only lower 3 bits out of 4 used)

#define GPUUNUSED0   0x0
#define GPUSETREG    0x1
#define GPUSETPALENT 0x2
#define GPUUNUSED3   0x3
#define GPUUNUSED4   0x4
#define GPUVMEMOUT   0x5
#define GPUGMEMOUT   0x6
#define GPUUNUSED7   0x7

inline void GPUSetRegister(const uint8_t reg, uint32_t val) {
    // Set lower 24 bits
    *IO_GPUCommandFIFO = GPUOPCODE24(GPUSETREG, reg, GPU24BITIMM(val));
    // Set upper 8 bits (instruction modifier==1 i.e.  0x8)
    *IO_GPUCommandFIFO = GPUOPCODE24((GPUSETREG|0x8), reg, GPU8BITIMMHI(val));
}

inline void GPUWriteGRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask) {
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUGMEMOUT, addrreg, valreg, (wmask&0xF));
}

inline void GPUWriteVRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask) {
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUVMEMOUT, addrreg, valreg, (wmask&0xF));
}

inline void GPUSetPaletteEntry(const uint8_t palindexreg, const uint8_t palcolorreg)
{
    // Use MAKERGBPALETTECOLOR() to pass packed colors to this function
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUSETPALENT, palindexreg, palcolorreg, 0);
}
