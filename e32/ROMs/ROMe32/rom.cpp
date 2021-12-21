// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <encoding.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // See https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf mcause section for the cause codes.

   uint32_t instr = read_csr(mtval);   // Instruction word
   uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16]
   uint32_t code = cause & 0x7FFFFFFF;

   if (cause & 0x80000000) // Interrupt
   {
      UARTWrite("\n\nINTERRUPT:  mcause = 0x\n");
      UARTWriteHex((uint32_t)cause);
      UARTWrite("\n");
   }
   else // Exception
   {
      switch (code)
      {
         // Illegal instruction
         case 2:
         {
            UARTWrite("\n\nEXCEPTION: Illegal instruction word 0x");
            UARTWriteHex((uint32_t)instr);
            UARTWrite("\n");
            // Stall
            while(1) { }
         }

         default:
         {
            UARTWrite("\n\nEXCEPTION:  mcause = 0x\n");
            UARTWriteHex((uint32_t)cause);
            UARTWrite("\n");
            // Stall
            while(1) { }
         }
      }
   }
}

void InstallIllegalInstructionHandler()
{
   // Set machine software interrupt handler
   swap_csr(mtvec, illegal_instruction_exception);

   // Enable machine software interrupt (breakpoint/illegal instruction)
   swap_csr(mie, MIP_MSIP);

   // Enable machine interrupts
   swap_csr(mstatus, MSTATUS_MIE);
}

int main()
{
   InstallIllegalInstructionHandler();

   UARTWrite("\033[2J\nE32 v0002\n\u00A9 2022 Engin Cilasun\n");

   while(1)
   {
      // Echo back incoming bytes
      if (*IO_UARTRXByteAvailable)
      {
         // Read incoming character
         uint8_t incoming = *IO_UARTRXTX;
         if (incoming == 'C') // Force crash to test exception handler
         {
            asm volatile(".dword 0x012345FF");
            asm volatile(".dword 0xFFFFFFFF");
         }
         // Write back to UART
         UARTFlush();
         *IO_UARTRXTX = incoming;
      }
   }

   return 0;
}
