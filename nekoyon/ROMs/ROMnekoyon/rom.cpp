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
#include "elf.h"

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
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
      UARTWrite("EXCEPTION: Illegal instruction I$(0x");
      UARTWriteHex((uint32_t)opcode);
      UARTWrite(") D$(0x");
      UARTWriteHex(*(uint32_t*)at);
      UARTWrite(") at 0x");
      UARTWriteHex((uint32_t)at);
      UARTWrite("\n");
   }
   else
   {
      // We've hit a breakpoint
      UARTWrite("EXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
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
      if (*IO_UARTRXByteAvailable)
         loadtargetaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      if (*IO_UARTRXByteAvailable)
         loadlenaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      if (*IO_UARTRXByteAvailable)
         target[writecursor++] = *IO_UARTRXTX;
   }
}

typedef int (*t_mainfunction)();

void LaunchELF(uint32_t branchaddress)
{
   // TODO: Set up return before we branch into this routine

   // Branch to loaded executable's entry point
   ((t_mainfunction)branchaddress)();
   /*asm (
      "lw x31, %0 \n"
      "jalr ra, 0(x31) \n"
      : 
      : "m" (branchaddress)
      : 
   );*/

   // Done, back in our world
   UARTWrite("Run complete.\n");
}

void FlushDataCache()
{
   // Force data cache flush so that D$ contents are visible by I$
   // We do this by forcing a dummy load of DWORDs from 0 to 2048
   // so that previous contents are evicted and written back.   
   for (uint32_t i=0; i<2048; ++i)
   {
      uint32_t dummyread = DDR3Start[i];
      // This is to make sure compiler doesn't eat our reads
      // and shuts up about unused variable
      asm volatile ("add x0, x0, %0;" : : "r" (dummyread) : );
   }
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
      uint32_t byteavailable = *IO_UARTRXByteAvailable;
      if (byteavailable != 0)
      {
         unsigned char readdata = *IO_UARTRXTX;
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   FlushDataCache();

   LaunchELF(branchaddress);

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

int main()
{
   InstallIllegalInstructionHandler();

   // Show startup info
   UARTWrite("\033[2J\r\n");
   UARTWrite("+-------------------------+\r\n");
   UARTWrite("|          ************** |\r\n");
   UARTWrite("| ########   ************ |\r\n");
   UARTWrite("| #########  ************ |\r\n");
   UARTWrite("| ########   ***********  |\r\n");
   UARTWrite("| #        ***********    |\r\n");
   UARTWrite("| ##   *************   ## |\r\n");
   UARTWrite("| ####   *********   #### |\r\n");
   UARTWrite("| ######   *****   ###### |\r\n");
   UARTWrite("| ########   *   ######## |\r\n");
   UARTWrite("| ##########   ########## |\r\n");
   UARTWrite("+-------------------------+\r\n");
   UARTWrite("\nNekoYon ROM version N4:0001\n");
   UARTWrite("rv32iZicsr\n");
   UARTWrite("Devices: UART,DDR3\n");
   UARTWrite("\u00A9 2021 Engin Cilasun\n\n");

   // UART communication section
   uint8_t prevchar = 0xFF;

   while(1)
   {
      // Step 1: Check to see if we have something in the FIFO
      if (*IO_UARTRXByteAvailable)
      {
         // Step 2: Read the data on UARTRX memory location
         char checkchar = *IO_UARTRXTX;

         // Step 3: Echo back what we've got
         // *IO_UARTRXTX = checkchar;

         // Step 4: Proceed to load binary blobs or run the incoming ELF
         // if we received a 'B\n' or 'R\n' sequence
         if (checkchar == 13)
         {
            // Load the incoming binary
            if (prevchar=='B')
               LoadBinaryBlob();
            if (prevchar=='R')
               RunBinaryBlob();
         }

         // Step 4: Remember this character for the next round
         prevchar = checkchar;
      }
   }

   return 0;
}
