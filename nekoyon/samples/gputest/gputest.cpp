#include "core.h"
#include "gpu.h"
#include "uart.h"

int main()
{
    UARTWrite("GPU functionality test\n\n");

    UARTWrite(" -resetting G-RAM contents to zero\n");
    __builtin_memset((void*)GRAMStart, 0x00, sizeof(uint32_t)*0xFFFF);

    // Create a command package
    UARTWrite(" -creating command buffer\n");
    GPUCommandPackage cmd;

    // Reset write cursor and write prologue
    GPUBeginCommandPackage(&cmd);

    // Choose page#0 for writes (and page#1 for display output)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE));           // vpage zero

    // Byte increment for VRAM writes
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R15, G_HIGHBITS, G_R15, 0x0000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R15, G_LOWBITS, G_R15, 0x0004));   // setregi r15, 0x00000004

    // Write 4 pixels to VRAM start
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, 0xFFFF));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, 0xFFFF));   // setregi r4, 0xFFFFFFFF
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R5, G_HIGHBITS, G_R5, 0x8000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R5, G_LOWBITS, G_R5, 0x0000));   // setregi r5, 0x80000000
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_STORE, G_R4, G_R5, 0x0, G_WORD));          // store.w r5, r4

    // Set next 4 pixels to another color
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, 0x0405));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, 0x0607));  // setregi r4, 0x04050607

    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_ALU, G_R5, G_R15, G_R5, G_ADD));          // add.w r5, r15, r5
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_STORE, G_R4, G_R5, 0x0, G_WORD));         // store.w r5, r4

    // Choose page#1 for writes (and page#0 for display output)
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R8, G_HIGHBITS, G_R8, 0x0000));
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_SETREG, G_R8, G_LOWBITS, G_R8, 0x0001));  // setregi r8, 0x00000001
    GPUWriteInstruction(&cmd, GPU_INSTRUCTION(G_MISC, G_R8, 0x0, 0x0, G_VPAGE));          // vpage r8

    // Write epilogue
    GPUEndCommandPackage(&cmd);

    UARTWrite(" -submitting command list\n");
    GPUClearMailbox();
    GPUSubmitCommands(&cmd);

    UARTWrite(" -validating commands\n");
    int has_errors = GPUValidateCommands(&cmd);
    if (has_errors == -1)
        UARTWrite(" -ERROR: command package errors detected\n");
    else
    {
        UARTWrite(" -kicking GPU work\n");
        GPUKick();

        UARTWrite(" -waiting for GPU write to mailbox\n");
        int retval = GPUWaitMailbox();

        if (retval == 0)
            UARTWrite(" -SUCCESS: GPU wrote to mailbox\n");
        else
            UARTWrite(" -ERROR: GPU mailbox write timed out\n");
    }

    return 0;
}