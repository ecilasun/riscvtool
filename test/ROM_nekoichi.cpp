// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#define STARTUP_ROM
#include "rom_nekoichi_rvcrt0.h"

#include "utils.h"
#include "gpu.h"

void loadbinaryblob()
{
   // Header data
   unsigned int loadlen = 0;
   unsigned int loadtarget = 0;
   char *loadlenaschar = (char*)&loadlen;
   char *loadtargetaschar = (char*)&loadtarget;

   // Target address
   unsigned int writecursor = 0;
   while(writecursor < 4)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadlenaschar[writecursor++] = readdata;
      }
   }

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         target[writecursor++] = readdata;
      }
   }
}

void runbinary()
{
   // Header data
   unsigned int branchaddress = 0;
   char *branchaddressaschar = (char*)&branchaddress;

   // Data length
   unsigned int writecursor = 0;
   while(writecursor < 4)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   // Old way: this doesn't set up anything before launching target executable
   // ((void (*)(void)) branchaddress)();

   // Set up stack pointer and branch to loaded executable's entry point (noreturn)
   asm (
      "lw ra, %0; \n"
      : 
      : "m" (branchaddress)
      : 
   );
   asm (
      "fmv.w.x	f0, zero; \n"
      "fmv.w.x	f1, zero; \n"
      "fmv.w.x	f2, zero; \n"
      "fmv.w.x	f3, zero; \n"
      "fmv.w.x	f4, zero; \n"
      "fmv.w.x	f5, zero; \n"
      "fmv.w.x	f6, zero; \n"
      "fmv.w.x	f7, zero; \n"
      "fmv.w.x	f8, zero; \n"
      "fmv.w.x	f9, zero; \n"
      "fmv.w.x	f10, zero; \n"
      "fmv.w.x	f11, zero; \n"
      "fmv.w.x	f12, zero; \n"
      "fmv.w.x	f13, zero; \n"
      "fmv.w.x	f14, zero; \n"
      "fmv.w.x	f15, zero; \n"
      "fmv.w.x	f16, zero; \n"
      "fmv.w.x	f17, zero; \n"
      "fmv.w.x	f18, zero; \n"
      "fmv.w.x	f19, zero; \n"
      "fmv.w.x	f20, zero; \n"
      "fmv.w.x	f21, zero; \n"
      "fmv.w.x	f22, zero; \n"
      "fmv.w.x	f23, zero; \n"
      "fmv.w.x	f24, zero; \n"
      "fmv.w.x	f25, zero; \n"
      "fmv.w.x	f26, zero; \n"
      "fmv.w.x	f27, zero; \n"
      "fmv.w.x	f28, zero; \n"
      "fmv.w.x	f29, zero; \n"
      "fmv.w.x	f30, zero; \n"
      "fmv.w.x	f31, zero; \n"
      "li x12, 0x0001FFF0; \n"
      "mv sp, x12; \n"
      "ret; \n"
   );

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

void echoterm(const char *_message)
{
   int i=0;
   while (_message[i]!=0)
   {
      UARTTX[i] = _message[i];
      ++i;
   }
}

int main()
{
   // 128 bytes of incoming command space
   char incoming[32];
   unsigned int rcvcursor = 0;

   echoterm("NekoIchi\r\nrv32imf@60Mhz\r\nv0001\r\n");

   int counter = 0;

   // Set page
   uint32_t page = 0;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(page));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(page));
   GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 1, 0, 0);

   // CLS
   uint8_t bgcolor = 0x03;//0x7C;
   uint32_t colorbits = (bgcolor<<24) | (bgcolor<<16) | (bgcolor<<8) | bgcolor;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(colorbits));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(colorbits));
   GPUFIFO[2] = GPUOPCODE(GPUCLEAR, 1, 0, 0);

   // Render text
   PrintDMA(56, 8, "Please upload ELF");
   PrintDMA(0, 16, "USB/UART @115200bps:8bit:1st:np");

   // Swap to other page to reveal previous render
   page = (page+1)%2;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(page));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(page));
   GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 1, 0, 0);

   // UART communication section
   while(1)
   {
      // Step 1: Read UART FIFO byte count
      unsigned int bytecount = UARTRXStatus[0];

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = incoming[rcvcursor++] = UARTRX[0];

         if (checkchar == 13) // Enter?
         {
            // Terminate the string
            incoming[rcvcursor-1] = 0;

            // Load the incoming binary
            if ((incoming[0]='b') && (incoming[1]=='i') && (incoming[2]=='n'))
               loadbinaryblob();
            if ((incoming[0]='r') && (incoming[1]=='u') && (incoming[2]=='n'))
               runbinary();
            // Rewind read cursor
            rcvcursor = 0;
         }

         // Echo the character back to the sender
         UARTTX[0] = checkchar;
         if (checkchar==13)
            UARTTX[1] = 10;

         // Wrap around if we're overflowing
         if (rcvcursor>31)
            rcvcursor = 0;
      }

      ++counter;
   }

   return 0;
}
