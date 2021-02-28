// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "rvcrt0.h"

#pragma GCC push_options
#pragma GCC optimize ("align-functions=16")

volatile unsigned int* VRAM = (volatile unsigned int* )0x80000000;       // Video Output: VRAM starts at 0, continues for 0xC000 bytes (256x192 8 bit packed color pixels, RGB[3:3:2] format)
volatile unsigned char* UARTTX = (volatile unsigned char* )0x40000000;     // UART send data (write)
volatile unsigned char* UARTRX = (volatile unsigned char* )0x50000000;     // UART receive data (read)
volatile unsigned int* UARTRXStatus = (volatile unsigned int* )0x60000000; // UART input status (read)
volatile unsigned int targetjumpaddress = 0x00000000;

unsigned int loadbinary()
{
   // Header data
   unsigned int loadlen = 0;
   unsigned int loadtarget = 0;
   char *loadlenaschar = (char*)&loadlen;
   char *loadtargetaschar = (char*)&loadtarget;

   // Data length
   unsigned int writecursor = 0;
   while(writecursor < 4)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadlenaschar[writecursor++] = readdata;
      }
   }

   // Target address
   writecursor = 0;
   while(writecursor < 4)
   {
      unsigned int bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Read binary data
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
         VRAM[scanline*64] = VRAM[scanline*64+63] = colorout;
         ++scanline;
         if (scanline>191) scanline = 0;
      }
   }

   return loadtarget;
}

void echoterm(const char *_message)
{
   int i=0;
   while (_message[i]!=0)
   {
      UARTTX[i] = _message[i];
      ++i;
   }
   UARTTX[0] = 13; // Also echo a CR+LF
   UARTTX[1] = 10; // Also echo a CR+LF
}

int main()
{
   // 128 bytes of incoming command space
   char incoming[32];

   unsigned int rcvcursor = 0;

   echoterm("ECRV32 v0.22");
   //print(0,0,12,"ECRV32 v0.2");

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

            // Help text
            if (incoming[0]='h' && incoming[1]=='e' && incoming[2]=='l' && incoming[3]=='p')
            {
               echoterm("\nrun[0x13][binsize][targetaddress][binarystream]");
            }

            // Run the incoming binary
            if (incoming[0]='r' && incoming[1]=='u' && incoming[2]=='n')
            {
               targetjumpaddress = loadbinary();
               /*asm (
                  "lw a5, %0;"
                  "jalr a5;" : : "m" (targetjumpaddress)
               );*/
               for(int a=0;a<192*64;++a)
                  VRAM[a] = 0x00000000;
               ((void (*)(void)) targetjumpaddress)();
               // Programs always load here
               /*asm (
                     "lui ra, 0x300;"
                     //"fence.i;"
                     "ret;"
                  );*/
            }

            // Rewind read cursor
            rcvcursor = 0;
         }

         UARTTX[0] = checkchar;
         if (checkchar==13)
            UARTTX[1] = 10;

         if (rcvcursor>31)
            rcvcursor = 0;
      }

      // Two scrolling color bars on each side of the screen
      VRAM[scanline*64] = VRAM[scanline*64+63] = (spincolor&0x000000FF) | ((spincolor&0x000000FF)<<8) | ((spincolor&0x000000FF)<<16) | ((spincolor&0x000000FF)<<24);
      if (clk>14 == 0)
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