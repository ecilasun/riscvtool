// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "rvcrt0.h"
#include "gpu.h"

#include "../nekoichiSDK/core.h"
#include "../nekoichiSDK/uart.h"

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

   EchoHex(loadlen);

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
         if ((writecursor % 8192) == 0)
            EchoUART(".");
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

   EchoUART("\nStarting\n");

   // Set up stack pointer and branch to loaded executable's entry point (noreturn)
   // TODO: Can we work out the stack pointer to match the loaded ELF's layout?
   asm (
      "lw ra, %0 \n"
      "li x12, 0x0FFFF000 \n" // we're not coming back to the loader, set SP to near the end of DDR3
      "mv sp, x12 \n"
      "ret \n"
      : 
      : "m" (branchaddress)
      : 
   );

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

int main()
{
    // Set color 0xFF to white
    GPUSetRegister(0, 0xFF);
    GPUSetRegister(1, MAKERGBPALETTECOLOR(0xFF,0xFF,0xFF));
    GPUSetPaletteEntry(0, 1);

    EchoUART("\033[2J\nNekoNi [v001] [RV32IZicsr]\n\u00A9 2021 Engin Cilasun\n");

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
         *IO_UARTTX = checkchar; // Echo back

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
