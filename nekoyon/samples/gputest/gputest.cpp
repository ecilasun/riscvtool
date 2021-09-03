#include "core.h"
#include "gpu.h"
#include "uart.h"

int main()
{
    UARTWrite("GPU functionality test\n\n");

    UARTWrite(" -resetting G-RAM contents to zero\n");
    __builtin_memset((void*)GRAMStart, 0x00, sizeof(uint32_t)*0xFFFF);

    // Create a 128 word command package
    UARTWrite(" -creating command buffer\n");
    GPUCommandPackage cmd;
    cmd.m_wordcount = 15;

    // Fill in the command buffer

//    _start:
//       halt                       // unconditional jump to 0x0000
    cmd.m_commands[0] = GPU_INSTRUCTION(G_FLOWCTL, 0x0, G_R0, 0x0, G_JMP);          // jmp zero

//    _main:
    // Set video write page to 0 (while we're seeing page 1)
    cmd.m_commands[1] = GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE);           // vpage zero
    // Write 4 pixels to VRAM start
    cmd.m_commands[2] = GPU_INSTRUCTION(G_SETREG, G_R4, G_HIGHBITS, G_R4, 0xFFFF);
    cmd.m_commands[3] = GPU_INSTRUCTION(G_SETREG, G_R4, G_LOWBITS, G_R4, 0xFFFF);   // setregi r4, 0xFFFFFFFF
    cmd.m_commands[4] = GPU_INSTRUCTION(G_SETREG, G_R5, G_HIGHBITS, G_R5, 0x8000);
    cmd.m_commands[5] = GPU_INSTRUCTION(G_SETREG, G_R5, G_LOWBITS, G_R5, 0x0000);   // setregi r5, 0x80000000
    cmd.m_commands[6] = GPU_INSTRUCTION(G_STORE, G_R4, G_R5, 0x0, G_WORD);          // store.w r5, r4
    // Set video write page to 1 (so that we can see page 0)
    cmd.m_commands[7] = GPU_INSTRUCTION(G_SETREG, G_R8, G_HIGHBITS, G_R8, 0x0000);
    cmd.m_commands[8] = GPU_INSTRUCTION(G_SETREG, G_R8, G_LOWBITS, G_R8, 0x0001);   // setregi r8, 0x00000001
    cmd.m_commands[9] = GPU_INSTRUCTION(G_MISC, G_R8, 0x0, 0x0, G_VPAGE);           // vpage r8

//    _exit:
//       store.w zero, zero         // write zero to address 0 (zero == register 0)
//       setregi.h 0x0000, r1, r1   // set register r1 to 0x0000FFF8 (mailbox address)
//       setregi.l 0xFFF8, r1, r1   //
//       store.w zero, r1           // write zero to address 0xFFF8
//       halt                       // unconditional jump to 0x0000
    cmd.m_commands[10] = GPU_INSTRUCTION(G_STORE, G_R0, G_R0, 0x0, G_WORD);          // store.w zero, zero
    cmd.m_commands[11] = GPU_INSTRUCTION(G_SETREG, G_R1, G_HIGHBITS, G_R1, 0x0000);
    cmd.m_commands[12] = GPU_INSTRUCTION(G_SETREG, G_R1, G_LOWBITS, G_R1, 0xFFF8);   // setregi r1, 0x0000FFF8
    cmd.m_commands[13] = GPU_INSTRUCTION(G_STORE, G_R0, G_R1, 0x0, G_WORD);          // store.w r1, zero
    cmd.m_commands[14] = GPU_INSTRUCTION(G_FLOWCTL, 0x0, G_R0, 0x0, G_JMP);          // jmp zero

    UARTWrite(" -submitting command list\n");
    GPUClearMailbox();
    GPUSubmitCommands(&cmd);

    UARTWrite(" -validating G-RAM contents\n");
    for (uint32_t i=0; i<cmd.m_wordcount; ++i)
    {
        if (GRAMStart[i] != cmd.m_commands[i])
        {
            UARTWrite("Mismatch @0x");
            UARTWriteHex(i*4);
            UARTWrite(": 0x");
            UARTWriteHex(GRAMStart[i]);
            UARTWrite(" != 0x");
            UARTWriteHex(cmd.m_commands[i]);
            UARTWrite("\n");
        }
    }

    UARTWrite(" -kicking GPU work\n");
    GPUKick();

    UARTWrite(" -waiting for GPU write to mailbox\n");
    int retval = GPUWaitMailbox();

    if (retval == 0)
        UARTWrite(" -SUCCESS: GPU wrote to mailbox\n");
    else
        UARTWrite(" -ERROR: GPU mailbox write timed out\n");

    return 0;
}