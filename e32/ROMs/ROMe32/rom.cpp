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
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   uint32_t at = read_csr(mtval);         // PC
   uint32_t cause = read_csr(mcause)>>16; // Exception cause on bits [18:16]

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0) // ILLEGALINSTRUCTION
   {
      // Show the address and the failing instruction's opcode field
      UARTWrite("\n\nEXCEPTION: Illegal instruction 0x");
      UARTWriteHex(*(uint32_t*)at);
      UARTWrite(" at 0x");
      UARTWriteHex((uint32_t)at);
      UARTWrite("\n");
      // Stall
      while(1) { }
   }

   if ((cause&2) != 0) // EBREAK
   {
      // We've hit a breakpoint
      // TODO: Tie this into GDB routines (connected via UART)
      UARTWrite("\n\nEXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
      // Stall
      while(1) { }
   }

   /*if ((cause&4) != 0) // ECALL
   {
      uint32_t ecalltype = 0;
      asm volatile ("sw a7, %0 ;" : "=m" (ecalltype) : :);

      // register a7 contains the function code
      // a7: 0x5D terminate application
      // a7: 0x50 seems to be fread

      if (ecalltype == 0x5D) // NOTE: We should not reach here under normal circumstances
      {
         // Now, store ra in stack for use by the target ELF
         asm volatile("lw ra, %0; ret;" : : "m" (returnaddress));
      }
      else
      {
         UARTWrite("\n\nECALL: 0x");
         UARTWriteHex(ecalltype);
         UARTWrite("\n");
      }
   }*/
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
