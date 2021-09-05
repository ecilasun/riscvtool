#include "inttypes.h"

// Instruction defines
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

// Helper macro to generate instruction words from instruction defines
#define GPU_INSTRUCTION(_op_, _rs1_, _rs2_, _rd_, _immed_) ((_op_) | (_rs1_<<4) | (_rs2_<<8) | (_rd_<<12) | (_immed_<<16))

// Helper macros to split words into halfwords
#define HIHALF(_x_) ((_x_&0xFFFF0000)>>16)
#define LOHALF(_x_) (_x_&0x0000FFFF)

// Program always has to start at this address
// 'halt' command will also always return here
#define GRAM_ADDRESS_PROGRAMSTART 0x0000
// Mailbox slot for GPU->CPU communication
#define GRAM_ADDRESS_GPUMAILBOX   0xFFF8

// A very simple, short command package with static command space allocated
struct GPUCommandPackage {
    volatile uint32_t m_commands[2048]; // Command list, 2048 instructions for now, limit is 64Kbytes-stack
    uint32_t m_wordcount{0};            // Length of command list in words
    uint32_t m_writecursor{0};          // Current write cursor
};

// Utility macro to build an RGB color to upload as a palette entry
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g)<<16) | ((_r)<<8) | (_b))

// Clean write cursor and submit count, and write prologue
void GPUBeginCommandPackage(GPUCommandPackage *_cmd);

// Close the command package, write epilogue and set submit word count
// If _noEpilogue is set to true, the program will be closed but won't
// contain code to signal CPU and halt itself. This is to be used when
// programs are to be chained together in the future.
void GPUEndCommandPackage(GPUCommandPackage *_cmd, bool _noEpilogue = false);

// Write instruction and return current cursor as a G-RAM address creation purposes
uint32_t GPUWriteInstruction(GPUCommandPackage *_cmd, const uint32_t _instruction);

// Scan closed command buffer and test for errors, return -1 if an error is found
int GPUValidateCommands(GPUCommandPackage *_cmd);

// Write 0xFFFFFFFF to the mailbox address so that the GPU can clear it when it's done
void GPUClearMailbox();

// Submit the commands in a closed command package
void GPUSubmitCommands(GPUCommandPackage *_cmd);

// Overwrite first instruciton with noop to cause GPU to start processing the submitted commmand package
// Can be repeated to re-kick the same command package if a change is not required
void GPUKick();

// Wait for GPU to write non-0xFFFFFFFF value (preferably zero) at the mailbox address, return -1 on timeout
int GPUWaitMailbox();
