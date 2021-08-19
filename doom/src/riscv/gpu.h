
#include <inttypes.h>

extern volatile uint32_t *IO_GPUCommandFIFO;
extern volatile uint32_t *GraphicsRAMStart;
extern volatile uint32_t *GraphicsRAMEnd;
extern volatile uint32_t *GraphicsFontStart;

// Utility macros
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g)<<16) | ((_r)<<8) | (_b))

// GPU command FIFO macros
#define GPU8BITIMMLO(_immed_) (_immed_ & 0x000000FF)
#define GPU24BITIMMHI(_immed_) ((_immed_ & 0xFFFFFF00) >> 8)
#define GPU20BITIMMLO(_immed_) (_immed_ & 0x000FFFFF)
#define GPUOPCODE20(_cmd_, _rd_, _rs_, _imm_) (_imm_<<12)|(_rs_<<8)|(_rd_<<4)|(_cmd_)
#define GPUOPCODE24(_cmd_, _rd_, _imm_) (_imm_<<8)|(_rd_<<4)|(_cmd_)

// GPU opcodes (only lower 3 bits out of 4 used)

#define GPUUNUSED0      0x00
#define GPUSETREGLO     0x01
#define GPUSETREGHI     0x09
#define GPUSETPALENT    0x02
#define GPUUNUSED3      0x03
#define GPUSYSDMA       0x04
#define GPUVMEMOUT      0x05
#define GPUGMEMOUT      0x06
#define GPUSETVPAGE     0x07

void GPUSetRegister(const uint8_t reg, uint32_t val);
void GPUSetVideoPage(const uint8_t videoPageRegister);
void GPUWriteGRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask);
void GPUWriteVRAM(const uint8_t addrreg, const uint8_t valreg, const uint8_t wmask);
void GPUSetPaletteEntry(const uint8_t palindexreg, const uint8_t palcolorreg);
void GPUKickDMA(const uint8_t SYSRAMSourceReg, const uint8_t VRAMDWORDAlignedTargetReg, const uint16_t DMALengthInDWORDs, const uint8_t masked);
