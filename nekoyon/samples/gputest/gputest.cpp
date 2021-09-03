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
    GPUCommandPackage *cmd = new GPUCommandPackage(6);

    // Fill in the commands
//    _start:
//       halt                       // unconditional jump to 0x0000
    cmd->m_commands[0] = GPU_INSTRUCTION(G_FLOWCTL, 0x0, 0x0, 0x0, 0x0000);

    // TODO: do something here

//    _exit:
//       store.w zero, zero         // write zero to address 0 (zero == register 0)
//       setregi.h 0x0000, r1, r1   // set register r1 to 0x0000FFF8 (mailbox address)
//       setregi.l 0xFFF8, r1, r1   //
//       store.w zero, r1           // write zero to address 0xFFF8
//       halt                       // unconditional jump to 0x0000
    cmd->m_commands[1] = GPU_INSTRUCTION(G_STORE, 0x0, 0x0, 0x0, 0x0000);
    cmd->m_commands[2] = GPU_INSTRUCTION(G_SETREG, 0x1, 0x0, 0x1, 0x0000); // rs2==0->set high16
    cmd->m_commands[3] = GPU_INSTRUCTION(G_SETREG, 0x1, 0x1, 0x1, 0xFFF8); // rs2==1->set low16
    cmd->m_commands[4] = GPU_INSTRUCTION(G_STORE, 0x0, 0x1, 0x0, 0x0000);
    cmd->m_commands[5] = GPU_INSTRUCTION(G_FLOWCTL, 0x0, 0x0, 0x0, 0x0000);

    UARTWrite(" -submitting commands\n");
    GPUClearMailbox();
    GPUSubmitCommands(cmd);
    GPUKick();

    UARTWrite(" -waiting for GPU write to mailbox\n");
    GPUWaitMailbox();

    UARTWrite(" -done\n");

    return 0;
}