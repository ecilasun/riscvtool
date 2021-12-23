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

static volatile int donotcrash = 0xDADED0D1;

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // See https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf mcause section for the cause codes.

   uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
   uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16]
   uint32_t code = cause & 0x7FFFFFFF;

   if (cause & 0x80000000) // Interrupt
   {
      if (code == 0xB) // hardware
      {
         // Echo back incoming bytes
         while (*IO_UARTRXByteAvailable)
         {
            // Read incoming character
            uint8_t incoming = *IO_UARTRXTX;
            // Force crash to test re-entering exception handler
            donotcrash = (incoming == 'C') ? 0x00000000 : 0xDADED0D1;
            // Write back to UART
            *IO_UARTRXTX = incoming;
            UARTFlush();
         }
      }
      if (code == 0x7) // timer
      {
         UARTWrite("tick...\n");
      }
   }
   else // Exception
   {
      switch (code)
      {
         case CAUSE_ILLEGAL_INSTRUCTION:
         {
            UARTWrite("\n\n\033[7mEXCEPTION: Illegal instruction word 0x");
            UARTWriteHex((uint32_t)value);
            UARTWrite("\n");
            // Stall
            while(1) { }
         }

         default:
         {
            UARTWrite("\n\n\033[7mEXCEPTION:  mcause = 0x");
            UARTWriteHex((uint32_t)cause);
            UARTWrite(" mtval = 0x");
            UARTWriteHex((uint32_t)value);
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

   // Enable machine software interrupts (breakpoint/illegal instruction)
   // Enable machine hardware interrupts
   swap_csr(mie, MIP_MSIP | MIP_MEIP);

   // Enable machine interrupts
   swap_csr(mstatus, MSTATUS_MIE);
}

int main()
{
   InstallIllegalInstructionHandler();

   // Clear all attributes, clear screen, print boot message
   UARTWrite("\033[0m\033[2J\nE32: RV32iZicsr\nROM v0003\n");

   while(1)
   {
      // Interrupt handler will do all the work

      // See if we're requested to forcibly crash
      if (donotcrash == 0x00000000)
      {
         UARTWrite("\nForcing illegal instruction exception...\n");
         asm volatile(".dword 0x012345FF");
         asm volatile(".dword 0xFFFFFFFF");
      }
   }

   return 0;
}
