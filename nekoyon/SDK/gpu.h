#include "inttypes.h"

#define G_FLOWCTL   0x0
#define G_SETREG    0x1
#define G_STORE     0x2
#define G_LOAD      0x3
#define G_DMA       0x4
#define G_WPAL      0x5
#define G_ALU       0x6
#define G_MISC      0x7
#define G_UNUSED0   0x8
#define G_UNUSED1   0x9
#define G_UNUSED2   0xA
#define G_UNUSED3   0xB
#define G_UNUSED4   0xC
#define G_UNUSED5   0xD
#define G_UNUSED6   0xE
#define G_UNUSED7   0xF

// branch/jmp
#define G_JMP 0x0
#define G_BNE 0x1
#define G_BEQ 0x2
#define G_BLE 0x3
#define G_BL  0x4
#define G_BG  0x5
#define G_BGE 0x6

// load/store
#define G_WORD 0x0
#define G_HALF 0x1
#define G_BYTE 0x2

// registers
#define G_R0  0x0
#define G_R1  0x1
#define G_R2  0x2
#define G_R3  0x3
#define G_R4  0x4
#define G_R5  0x5
#define G_R6  0x6
#define G_R7  0x7
#define G_R8  0x8
#define G_R9  0x9
#define G_R10 0xA
#define G_R11 0xB
#define G_R12 0xC
#define G_R13 0xD
#define G_R14 0xE
#define G_R15 0xF


// setregi.h/l
#define G_HIGHBITS 0x0
#define G_LOWBITS  0x1

// ALU ops
#define G_CMP 0x0
#define G_SUB 0x1
#define G_DIV 0x2
#define G_MUL 0x3
#define G_ADD 0x4
#define G_AND 0x5
#define G_OR  0x6
#define G_XOR 0x7

// noop / vsync / vpage
#define G_NOOP  0x0
#define G_VSYNC 0x1
#define G_VPAGE 0x2

#define GPU_INSTRUCTION(_op_, _rs1_, _rs2_, _rd_, _immed_) ((_op_) | (_rs1_<<4) | (_rs2_<<8) | (_rd_<<12) | (_immed_<<16))

// Program always has to start at this address
// 'halt' command will also always return here
#define GRAM_ADDRESS_PROGRAMSTART 0x0000
// Mailbox slot for GPU->CPU communication
#define GRAM_ADDRESS_GPUMAILBOX   0xFFF8

struct GPUCommandPackage {
    volatile uint32_t m_commands[512];  // Command list, 512 instructions for now, limit is 64Kbytes-stack
    uint32_t m_wordcount{0};            // Length of command list in words
    uint32_t m_writecursor{0};          // Current write cursor
};

void GPUBeginCommandPackage(GPUCommandPackage *_cmd);
void GPUEndCommandPackage(GPUCommandPackage *_cmd);
void GPUWriteInstruction(GPUCommandPackage *_cmd, const uint32_t _instruction);
int GPUValidateCommands(GPUCommandPackage *_cmd);

void GPUClearMailbox();
void GPUSubmitCommands(GPUCommandPackage *_cmd);
void GPUKick();
int GPUWaitMailbox(); // -1 on timeout
