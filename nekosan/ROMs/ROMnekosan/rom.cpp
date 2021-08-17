// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include <encoding.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"
#include "leds.h"

// NOTE: Trap handler's always aligned to 256 bytes
void __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   uint32_t at = read_csr(mtval);
   uint32_t cause = read_csr(mcause);

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0)
   {
      // Offending instruction's opcode field
      uint32_t opcode = read_csr(mscratch);

      // Show the address and the failing instruction's opcode field
      EchoStr("EXCEPTION: Illegal instruction I$(0x");
      EchoHex((uint32_t)opcode);
      EchoStr(") D$(0x");
      EchoHex(*(uint32_t*)at);
      EchoStr(") at 0x");
      EchoHex((uint32_t)at);
      EchoStr("\n");
   }
   else
   {
      // We've hit a breakpoint
      EchoStr("EXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
   }

   // Deadlock
   while(1) { }
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

void LoadBinaryBlob()
{
   // Header data
   uint32_t loadlen = 0;
   uint32_t loadtarget = 0;
   char *loadlenaschar = (char*)&loadlen;
   char *loadtargetaschar = (char*)&loadtarget;

   // Target address
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
         loadtargetaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
         loadlenaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Read binary blob
   writecursor = 0;
   volatile uint8_t* target = (volatile uint8_t* )loadtarget;
   while(writecursor < loadlen)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         target[writecursor++] = *IO_UARTRXTX;
         *IO_LEDRW = (writecursor>>9)&0x1; // Blink rightmost LED as status indicator
      }
   }
   *IO_LEDRW = 0x0; // Turn all LEDs off
}

void RunBinaryBlob()
{
   // Header data
   uint32_t branchaddress = 0;
   char *branchaddressaschar = (char*)&branchaddress;

   // Data length
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
         branchaddressaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Set up stack pointer and branch to loaded executable's entry point (noreturn)
   // NOTE: Assuming a one-way trip here for now since we live in ARAM and might
   // be overwritten with audio data.
   asm (
      //"mv x1, x0\n" // Return address will be set below
      //"mv x2, x0\n" // Stack pointer will be set below
      "mv x3, x0\n"
      "mv x4, x0\n"
      "mv x5, x0\n"
      "mv x6, x0\n"
      "mv x7, x0\n"
      "mv x8, x0\n"
      "mv x9, x0\n"
      "mv x10, x0\n"
      "mv x11, x0\n"
      "mv x12, x0\n"
      "mv x13, x0\n"
      "mv x14, x0\n"
      "mv x15, x0\n"
      "mv x16, x0\n"
      "mv x17, x0\n"
      "mv x18, x0\n"
      "mv x19, x0\n"
      "mv x20, x0\n"
      "mv x21, x0\n"
      "mv x22, x0\n"
      "mv x23, x0\n"
      "mv x24, x0\n"
      "mv x25, x0\n"
      "mv x26, x0\n"
      "mv x27, x0\n"
      "mv x28, x0\n"
      "mv x29, x0\n"
      "mv x30, x0\n"
      "mv x31, x0\n"
      "lw ra, %0 \n"
      "li x12, 0x0FFFF000 \n" // End of DDR3 - NOTE: On NekoSan, stack stays on ARAM
      "mv sp, x12 \n"
      "ret \n"
      : 
      : "m" (branchaddress)
      : 
   );
}

int main()
{
   // Turn off all LEDs
   *IO_LEDRW = 0x0;

   // Illegal instruction trap
   InstallIllegalInstructionHandler();

   // Show startup info
   EchoStr("\033[2J\r\n");
   EchoStr("+-------------------------+\r\n");
   EchoStr("|          ************** |\r\n");
   EchoStr("| ########   ************ |\r\n");
   EchoStr("| #########  ************ |\r\n");
   EchoStr("| ########   ***********  |\r\n");
   EchoStr("| #        ***********    |\r\n");
   EchoStr("| ##   *************   ## |\r\n");
   EchoStr("| ####   *********   #### |\r\n");
   EchoStr("| ######   *****   ###### |\r\n");
   EchoStr("| ########   *   ######## |\r\n");
   EchoStr("| ##########   ########## |\r\n");
   EchoStr("+-------------------------+\r\n");
   EchoStr("\nNekoSan version 0001\n");
   EchoStr("RV32IMZicsr\n");
   EchoStr("Devices: UART, SPI, SWTCH, LED\n");
   EchoStr("\u00A9 2021 Engin Cilasun\n\n");

   // UART communication section
   uint8_t prevchar = 0xFF;
   while(1)
   {
      // Step 1: Read UART FIFO byte count
      uint32_t bytecount = *IO_UARTRXByteCount;

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = *IO_UARTRXTX;

         // Step 4: Echo back as debug
         // *IO_UARTRXTX = checkchar;

         // Step 5: Load binary blob or run the incoming ELF
         if (checkchar == 13) // B or R followed by enter
         {
            // Load the incoming binary
            if (prevchar=='B')
               LoadBinaryBlob();
            if (prevchar=='R')
               RunBinaryBlob();
         }
         prevchar = checkchar;
      }
   }

   return 0;
}
