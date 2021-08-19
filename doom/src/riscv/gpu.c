#include "gpu.h"
#include "config.h"

volatile uint32_t *IO_GPUCommandFIFO = (volatile uint32_t* )0x80000000;   // GPU command FIFO
volatile uint32_t *GraphicsRAMStart = (uint32_t* )0x10000000;             // Start of Graphics RAM region (inclusive, 64Bytes)
volatile uint32_t *GraphicsRAMEnd = (uint32_t* )0x10010000;               // End of Graphics RAM region (non-inclusive)

void GPUSetRegister(const uint8_t reg, uint32_t val) {
    // Set upper 24 bits
    *IO_GPUCommandFIFO = GPUOPCODE24(GPUSETREGHI, reg, GPU24BITIMMHI(val));
    // Set lower 8 bits
    *IO_GPUCommandFIFO = GPUOPCODE24(GPUSETREGLO, reg, GPU8BITIMMLO(val));
}

void GPUSetVideoPage(const uint8_t videoPageRegister)
{
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUSETVPAGE, 0, videoPageRegister, 0);
}

void GPUWriteGRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask) {
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUGMEMOUT, addrreg, valreg, (wmask&0xF));
}

void GPUWriteVRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask) {
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUVMEMOUT, addrreg, valreg, (wmask&0xF));
}

void GPUSetPaletteEntry(const uint8_t palindexreg, const uint8_t palcolorreg) {
    // Use MAKERGBPALETTECOLOR() to pass packed colors to this function
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUSETPALENT, palindexreg, palcolorreg, 0);
}

void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked)
{
    *IO_GPUCommandFIFO = GPUOPCODE20(GPUSYSDMA, VRAMDWORDAlignedTargetReg, SYSRAMSourceReg, ((DMALengthInDWORDs&0x7FFF) | (masked ? 0x8000 : 0x0000)));
}
