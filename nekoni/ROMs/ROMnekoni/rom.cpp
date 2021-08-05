// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "rvcrt0.h"
#include "gpu.h"

#include "core.h"
#include "uart.h"

void __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   // Grab address of illegal instruction exception
   register uint32_t at;
   asm volatile("csrr %0, mtval" : "=r"(at));

   // Show the address and the failing instruction
   EchoUART("Illegal instruction @0x");
   EchoHex((uint32_t)at);
   EchoUART(" 0x");
   EchoHex(*(uint32_t*)at);
   EchoUART(" 0x\n");

   // Deadlock
   while(1) { }
}

void InstallIllegalInstructionHandler()
{
   int msie = (1 << 2); // Enable illegal instruction exception
   int mstatus = (1 << 3); // Enable machine interrupts

   asm volatile("csrrw zero, mtvec, %0" :: "r" (illegal_instruction_exception));
   asm volatile("csrrw zero, mie,%0" :: "r" (msie));
   asm volatile("csrrw zero, mstatus,%0" :: "r" (mstatus));
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
      {
         unsigned char readdata = *IO_UARTRX;
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         loadlenaschar[writecursor++] = readdata;
      }
   }

   int percent = 0;
   long chunks = (loadlen+511)/512;

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         target[writecursor++] = readdata;
      }
      if ((writecursor%512) == 0)
      {
         EchoUART("\0337 Loading [");
         for (int i=0;i<percent;++i)
            EchoUART(":");
         for (int i=0;i<20-percent;++i)
            EchoUART(" ");
         EchoUART("] \0338");
         percent = (20*(writecursor+511)/512)/chunks;
      }
   }
   EchoUART("\n");
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
      {
         unsigned char readdata = *IO_UARTRX;
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   EchoUART("\nstarting...\n");

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
      "li x12, 0x0FFFF000 \n" // End of DDR3
      "mv sp, x12 \n"
      "ret \n"
      : 
      : "m" (branchaddress)
      : 
   );
}

/*void DDR3CacheTest()
{
   // Write something on tag #800 line #0
   for (uint32_t m=0x00010000; m<0x00010020; m+=4)
      *(uint32_t*)m = 0xFAFEF0F1 ^ m;

   // Write something on tag #808 line #0 (overlaps same cache line, forces flush)
   for (uint32_t m=0x01010000; m<0x01010020; m+=4)
      *(uint32_t*)m = 0xFAFEF0F1 ^ m;

   // Go back to initial tag #800 to see if what we've written is still intact
   for (uint32_t m=0x00010000; m<0x00010020; m+=4)
   {
      EchoUART(":");
      EchoHex(*(uint32_t*)m);
      EchoUART("\n");
   }
}*/

int main()
{
   InstallIllegalInstructionHandler();

   //DDR3CacheTest();

   EchoUART("\033[2J\nNekoNi [v002] [RV32IZicsr]+[GPU]\n\u00A9 2021 Engin Cilasun\n");

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
         char checkchar = *IO_UARTRX;

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
