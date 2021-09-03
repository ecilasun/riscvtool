#include "core.h"
#include "gpu.h"

#include <stdlib.h>

// NOTE: All memory addresses referred to in this section are in G-RAM address space (0x20000000-0x20010000)
// unless otherwise stated

// 1 - If GPU encounters a HALT instruction anywhere in memory (default memory contents), it branch to 0x0000
//     effectively pausing execution. (HALT encodes as jmp zero so it's an unconditional branch to top of G-RAM)
// 2 - To achieve this, first thing the CPU does is to write 0xFFFFFFFF to address 0xFFF8 to clear the mailbox
// 3 - CPU then copies contents of _cmd->m_commands to address 0x0000 for _cmd->m_wordcount words
// 4 - Once copy is complete, CPU writes 'noop' to address 0x0000 replacing the existing 'halt'
//     command. This will make the GPU get unstuck and start processing instructions.
// 5 - At the end of every program, there are few instructions to write a HALT to 0x0000 and
//     zero to 0xFFF8, then branch to 0x0000.
//     CPU waits for a non-zero mailbox value to become non-zero to submit another list of commands.
//
//     Function prologue
//     start:
//       halt                       // unconditional jump to 0x0000
//
//     Function epilogue
//     programend:
//       store.w zero, zero         // write zero to address 0 (zero == register 0)
//       setregi.h 0x0000, r1, r1   // set register r1 to 0x0000FFF8 (mailbox address)
//       setregi.l 0xFFF8, r1, r1   //
//       store.w zero, r1           // write zero to address 0xFFF8
//       halt                       // unconditional jump to 0x0000

void GPUClearMailbox()
{
    GRAMStart[GRAM_ADDRESS_GPUMAILBOX>>2] = 0xFFFFFFFF;
}

void GPUHalt()
{
    // jmp zero to block GPU - NOTE: This is already built-in into each program
    GRAMStart[GRAM_ADDRESS_PROGRAMSTART>>2] = GPU_INSTRUCTION(G_FLOWCTL, 0x0, G_R0, 0x0, G_JMP);
}

void GPUKick()
{
    // noop to unblock GPU
    GRAMStart[GRAM_ADDRESS_PROGRAMSTART>>2] = GPU_INSTRUCTION(G_MISC, 0x0, 0x0, 0x0, G_NOOP);
}

void GPUSubmitCommands(GPUCommandPackage *_cmd)
{
    // NOTE: MUST NOT call this function a second time without a GPUWaitMailBox() in between
    // otherwise currently running program may hang/get corrupted.

    // Copy the program to G-RAM. First word must by default be a HALT instruction,
    // otherwise we risk immediate execution of the program, therefore a hang or corruption.
    __builtin_memcpy((void*)GRAMStart, (void*)_cmd->m_commands, sizeof(uint32_t)*_cmd->m_wordcount);
}

int GPUWaitMailbox()
{
    uint32_t waitcounter = 0x0;
    // Ensure that the last instruction in the GPU program wrote a zero to the mailbox
    while (GRAMStart[GRAM_ADDRESS_GPUMAILBOX>>2] == 0xFFFFFFFF) {
        //asm volatile ("nop;");
        if (waitcounter++ > 131072)
            return -1;
    }
    return 0;
}
