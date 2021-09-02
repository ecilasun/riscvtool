#include "inttypes.h"

#define G_HALTNOOP  0x0
#define G_SETREG    0x1
#define G_STORE     0x2
#define G_STORE     0x3
#define G_DMA       0x4
#define G_WPAL      0x5
#define G_ALU       0x6
#define G_FLOW      0x7
#define G_ALUI      0x8
#define G_RET       0x9
#define G_VSYNC     0xA
#define G_UNUSED0   0xB
#define G_UNUSED1   0xC
#define G_UNUSED2   0xD
#define G_UNUSED3   0xE
#define G_UNUSED4   0xF


#define GPU_INSTRUCTION(_op_, _rs1_, _rs2_, _rd_, _immed_) ((_op_) | (_rs1_<<4) | (_rs2_<<8) | (_rd_<<12) | (_immed_<<16))

struct GPUCommandPackage {
    volatile uint32_t *m_commands;
    uint32_t m_length;
};
