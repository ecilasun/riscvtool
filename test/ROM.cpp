// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "rom_rvcrt0.h"
#include "utils.h"

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

void loadbinaryblob()
{
   volatile uint32_t *VRAMDW = (uint32_t *)VRAM;

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

   // Binary blob
   writecursor = 0;
   unsigned int scanline = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      unsigned int colorout = 0x00000000;
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         target[writecursor++] = readdata;

         colorout = readdata | (readdata<<8) | (readdata<<16) | (readdata<<24);
         VRAMDW[scanline*64] = VRAMDW[scanline*64+63] = colorout;
         ++scanline;
         if (scanline>191) scanline = 0;
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

   // Jump to entry point
   ((void (*)(void)) branchaddress)();

   // Or alternatively, set 'ra' to branchaddress and 'ret' since we won't be exiting main()
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
   volatile uint32_t *VRAMDW = (uint32_t *)VRAM;
   uint32_t clearcolor = 0xFFFFFFFF;
   for(uint32_t i=0;i<192*64;++i) VRAMDW[i]=clearcolor;

   // 128 bytes of incoming command space
   char incoming[32];
   unsigned int rcvcursor = 0;

   echoterm("v0.25 rv32im@140Mhz\r\n");

   // UART communication section
   int scanline = 0;
   int spincolor = 0;
   int clk = 0;
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
            if (incoming[0]='b' && incoming[1]=='i' && incoming[2]=='n')
               loadbinaryblob();
            if (incoming[0]='r' && incoming[1]=='u' && incoming[2]=='n')
               runbinary();
            // Rewind read cursor
            rcvcursor = 0;
         }

         // Echo the character back to the sender
         /*UARTTX[0] = checkchar;
         if (checkchar==13)
            UARTTX[1] = 10;*/

         // Wrap around if we're overflowing
         if (rcvcursor>31)
            rcvcursor = 0;
      }

      // Show two scrolling color bars on each side of the screen as 'alive' indicator
      VRAMDW[scanline*64] = VRAMDW[scanline*64+63] = (spincolor&0x000000FF) | ((spincolor&0x000000FF)<<8) | ((spincolor&0x000000FF)<<16) | ((spincolor&0x000000FF)<<24);
      if (clk > 130944) // 682*192
      {
         ++spincolor;
         clk = 0;
      }
      ++clk;
      ++scanline;
      if (scanline>191)
         scanline = 0;
   }

   return 0;
}

#pragma GCC pop_options