// MiniTerm
// Send to the SoC using:
// .\build\release\riscvtool.exe .\miniterm.elf -sendelf 0x00000210

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "umm_malloc.h"
#include "utils.h"
#include "SDCARD.h"
#include "FAT.h"

// #pragma GCC push_options
// #pragma GCC optimize ("align-functions=16")

static int s_filesystemready = 0;
volatile unsigned int targetjumpaddress = 0x00000000;
char chartable[32*24];

void clearchars()
{
   for (int cy=0;cy<24;++cy)
      for (int cx=0;cx<32;++cx)
         chartable[cx+cy*32] = ' ';
}

void scroll()
{
   for (int cy=0;cy<23;++cy)
      for (int cx=0;cx<32;++cx)
         chartable[cx+cy*32] = chartable[cx+(cy+1)*32];

   // Clear the last row
   for (int cx=0;cx<32;++cx)
      chartable[cx+23*32] = ' ';
}

void loadelf(char *commandline)
{
   FILEHANDLE fh;
   if (fat_openfile(&commandline[5], &fh))
   {
      EchoUART("\r\nReading file...\r\n");
      uint32_t fsz = fat_getfilesize(&fh);
      EchoInt(fsz);
      char buffer[4096]; // AT LEAST 4096! (spc(8)*bps(512)==4096)
      int readsize;
      fat_readfile(&fh, buffer, fsz, &readsize);
      buffer[fsz] = 0;
      EchoUART(buffer);
      fat_closefile(&fh);
      EchoUART("Done\r\n");
   }
   else
      EchoUART("\r\nFile not found\r\n");
}

int main()
{
   if (SDCardStartup() != -1)
   {
      fat_getpartition();
      s_filesystemready = 1;
   }

   const unsigned char bgcolor = 0xFF; // BRG -> B=0xC0, R=0x38, G=0x07
   //const unsigned char editbgcolor = 0x00;

   // 32 bytes of incoming command space
   char incoming[32];
   //char *incoming = (char*)malloc(32);

   unsigned int rcvcursor = 0;
   unsigned int cmdcounter = 23;
   //unsigned int oldcount = 0;

   // Startup message
   clearchars();
   ClearScreen(bgcolor);
   PrintMasked(0, 176, " MiniTerm (c)2021 Engin Cilasun ");

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

         if (checkchar == 8 && rcvcursor!=1) // Backspace? (make sure your terminal uses ctrl+h for backspace)
         {
            ClearScreen(bgcolor);
 
            --rcvcursor;
            incoming[rcvcursor-1] = 0;
            // Copy the string to the chartable
            for (unsigned int i=0;i<rcvcursor;++i)
               chartable[i+cmdcounter*32] = incoming[i];
             --rcvcursor;
         }
         else if (checkchar == 13) // Enter?
         {
            // Terminate the string
            incoming[rcvcursor-1] = 0;

            // Copy the string to the chartable
            for (unsigned int i=0;i<rcvcursor;++i)
               chartable[i+cmdcounter*32] = incoming[i];
            
            int canclear = 1;

            // Clear the whole screen
            if ((incoming[0]='c') && (incoming[1]=='l') && (incoming[2]=='s'))
            {
               clearchars();
            }
            else if ((incoming[0]='h') && (incoming[1]=='e') && (incoming[2]=='l') && (incoming[3]=='p'))
            {
               EchoUART("\n\rMiniTerm version 0.1\n\r(c)2021 Engin Cilasun\n\rdir: list files at root directory\r\nload filename: load and run ELF\n\rcls: clear screen\n\rhelp: help screen\n\rreset: reboot to loader\n\r");
            }
            else if ((incoming[0]='d') && (incoming[1]=='i') && (incoming[2]=='r'))
            {
               // Attempt to re-start file system if not ready
               if (!s_filesystemready)
               {
                  if (SDCardStartup() != -1)
                  {
                     fat_getpartition();
                     s_filesystemready = 1;
                  }
               }

               if (s_filesystemready)
               {
                  char target[512] = "\r\n";
                  fat_list_files(target);
                  EchoUART(target);
               }
               else
                  EchoUART("\r\nFile system not ready\r\n");
            }
            else if ((incoming[0]='l') && (incoming[1]=='o') && (incoming[2]=='a') && (incoming[3]=='d'))
            {
               loadelf(incoming);
            }
            else if ((incoming[0]='r') && (incoming[1]=='e') && (incoming[2]=='s') && (incoming[3]=='e') && (incoming[4]=='t'))
            {
               ((void (*)(void)) ROMResetVector)();
            }

            if(canclear)
               ClearScreen(bgcolor);

            scroll();

            // Rewind read cursor
            rcvcursor=0;
         }
         else
         {
            // Print out this many characters
            PrintMasked(0, 184, rcvcursor, incoming);
         }

         // Show the char table
         for (int cy=0;cy<24;++cy)
            for (int cx=0;cx<32;++cx)
               PrintMasked(8*cx, 8*cy, 1, &chartable[cx+cy*32]);

         // Echo characters back to the terminal
         UARTTX[0] = checkchar;
         if (checkchar == 13)
            UARTTX[0] = 10; // Echo a linefeed

         if (rcvcursor>31)
            rcvcursor = 0;
      }
   }

   //free(incoming);

   return 0;
}

// #pragma GCC pop_options
