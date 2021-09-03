#include "inttypes.h"

#define G_FLOWCTL   0x0
#define G_SETREG    0x1
#define G_STORE     0x2
#define G_LOAD      0x3
#define G_DMA       0x4
#define G_WPAL      0x5
#define G_ALU       0x6
#define G_NOOP      0x7
#define G_ALUI      0x8
#define G_UNUSED0   0x9
#define G_VSYNC     0xA
#define G_UNUSED1   0xB
#define G_UNUSED2   0xC
#define G_UNUSED3   0xD
#define G_UNUSED4   0xE
#define G_UNUSED5   0xF


#define GPU_INSTRUCTION(_op_, _rs1_, _rs2_, _rd_, _immed_) ((_op_) | (_rs1_<<4) | (_rs2_<<8) | (_rd_<<12) | (_immed_<<16))

#define GRAM_ADDRESS_PROGRAMSTART 0x0000
#define GRAM_ADDRESS_GPUMAILBOX   0xFFF8

class GPUCommandPackage {
public:
    explicit GPUCommandPackage(uint32_t _bufferLen);
    ~GPUCommandPackage();
    volatile uint32_t *m_commands;  // Command list
    uint32_t m_wordcount;           // Length of command list in words
};

void GPUClearMailbox();
void GPUSubmitCommands(GPUCommandPackage *_cmd);
void GPUKick();
void GPUWaitMailbox();
