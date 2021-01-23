// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "rvcrt0.h"

volatile unsigned char* UARTTX = (volatile unsigned char* )0x40000000;     // UART send data (write)
volatile unsigned char* UARTRX = (volatile unsigned char* )0x50000000;     // UART receive data (read)
volatile unsigned int* UARTRXStatus = (volatile unsigned int* )0x60000000; // UART input status (read)
volatile unsigned int* VRAM = (volatile unsigned int* )0x80000000;       // Video Output: VRAM starts at 0, continues for 0xC000 bytes (256x192 8 bit packed color pixels, RGB[3:3:2] format)
volatile unsigned int targetjumpaddress = 0x00000000;

unsigned int load()
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

   return loadtarget;
}

void cls(uint32_t val)
{
   for(int a=0;a<192*256/4;++a)
      VRAM[a] = val;
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
   char incoming[128];

   uint32_t val1 = 0x38000000; // BRG -> B=0xC0, R=0x38, G=0x07
   uint32_t val2 = 0x00000038; // BRG -> B=0xC0, R=0x38, G=0x07
   unsigned int rcvcursor = 0;
   unsigned int oldcount = 0;
   unsigned int ticker = 0;

   cls(0xC0C0C0C0);
   echoterm("ECRV32 v0.1");

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

            // Rewind read cursor
            rcvcursor = 0;

            // Run the incoming binary
            if (incoming[0]='r' && incoming[1]=='u' && incoming[2]=='n')
            {
               targetjumpaddress = load();
               ((void (*)(void)) targetjumpaddress)();
            }
         }

         if (rcvcursor>126)
            rcvcursor = 0;
      }

      // Tick animation
      ++ticker;
      if (ticker > 131072)
      {
         ticker = 0;
         val1 = (val1>>8) | ((val1&0x000000FF)<<24);
         val2 = ((val2&0x00FFFFFF)<<8) | ((val2>>24)&0x000000FF);
         for(int a=0;a<4*256/4;++a)
            VRAM[92*256/4+a] = val1;
         for(int a=0;a<4*256/4;++a)
            VRAM[96*256/4+a] = val2;
      }
   }

   return 0;
}
