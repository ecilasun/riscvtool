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

   //int percent = 0;
   //long chunks = (loadlen+511)/512;
   UARTWrite("\0337 Loading @0x");
   UARTWriteHex(loadtarget);
   UARTWrite("\n");

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

   UARTWrite("\nStarting @0x");
   UARTWriteHex(branchaddress);
   UARTWrite("\n");

   LaunchELF(branchaddress);

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

int main()
{
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
   UARTWrite("\nNekoYon version 0001\n");
   UARTWrite("rv32i\n");
   UARTWrite("Devices: UART\n");
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
         *IO_UARTRXTX = checkchar;

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
