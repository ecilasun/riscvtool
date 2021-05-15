// MiniTerm

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"
#include "gpu.h"
#include "SDCARD.h"
#include "FAT.h"
#include "console.h"

static int s_filesystemready = 0;
volatile unsigned int targetjumpaddress = 0x00000000;

void loadelf(char *commandline)
{
   FILEHANDLE fh;
   if (fat_openfile(&commandline[5], &fh))
   {
      EchoConsole("\r\nReading file...\r\n");
      uint32_t fsz = fat_getfilesize(&fh);
      EchoInt(fsz);
      char buffer[4096]; // AT LEAST 4096! (spc(8)*bps(512)==4096)
      int readsize;
      fat_readfile(&fh, buffer, fsz>4096?4096:fsz, &readsize);
      buffer[fsz] = 0;
      EchoConsole(buffer);
      fat_closefile(&fh);
      EchoConsole("Done\r\n");
   }
   else
   {
      EchoConsole("\r\nFile ");
      EchoConsole(&commandline[5]);
      EchoConsole(" not found\r\n");
   }
}

int main()
{
   if (SDCardStartup() != -1)
   {
      fat_getpartition();
      s_filesystemready = 1;
   }

   const unsigned char bgcolor = 0xC0; // BRG -> B=0xC0, R=0x38, G=0x07
   //const unsigned char editbgcolor = 0x00;

   char incoming[36];
   //char *incoming = (char*)malloc(32);

   unsigned int rcvcursor = 0;
   //unsigned int oldcount = 0;

   // Set output page
   uint32_t page = 0;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 6, GPU22BITIMM(page));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 6, 6, GPU10BITIMM(page));
   GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 6, 0, 0);

   // Startup message
   ClearConsole();
   ClearScreen(bgcolor);
   EchoConsole(" MiniTerm (c)2021 Engin Cilasun\r\n Use 'help' for assistance\r\n");
   EchoUART(" MiniTerm (c)2021 Engin Cilasun\r\n UART echo is off\r\n");
   DrawConsole();

   page = (page+1)%2;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 6, GPU22BITIMM(page));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 6, 6, GPU10BITIMM(page));
   GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 6, 0, 0);

   // UART communication section
   uint32_t prevmilliseconds = 0;
   uint32_t cursorevenodd = 0;
   uint32_t toggletime = 0;
   volatile unsigned int gpustate = 0x00000000;
   unsigned int cnt = 0x00000000;
   while(1)
   {
      uint64_t clk = ReadClock();
      uint32_t milliseconds = ClockToMs(clk);
      uint32_t hours, minutes, seconds;
      ClockMsToHMS(milliseconds, hours,minutes,seconds);

      // Step 1: Read UART FIFO byte count
      unsigned int bytecount = UARTRXStatus[0];

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = incoming[rcvcursor++] = UARTRX[0];

         if (checkchar == 8) // Backspace? (make sure your terminal uses ctrl+h for backspace)
         {
            if (rcvcursor!=1)
            {
               --rcvcursor;
               incoming[rcvcursor] = 0;
               incoming[rcvcursor-1] = 0;
               --rcvcursor;
            }
            ConsoleCursorStepBack();
            EchoConsole(" ");
            ConsoleCursorStepBack();
         }
         else if (checkchar == 13) // Enter?
         {
            EchoConsole("\n"); // New line

            // Clear the whole screen
            if ((incoming[0]='c') && (incoming[1]=='l') && (incoming[2]=='s'))
            {
               ClearConsole();
            }
            else if ((incoming[0]='h') && (incoming[1]=='e') && (incoming[2]=='l') && (incoming[3]=='p'))
            {
               EchoConsole("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ndir: list files\r\nload filename: load and run ELF\n\rcls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\n");
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
                  EchoConsole(target);
               }
               else
                  EchoConsole("\r\nFile system not ready\r\n");
            }
            else if ((incoming[0]='l') && (incoming[1]=='o') && (incoming[2]=='a') && (incoming[3]=='d'))
            {
               loadelf(incoming);
            }
            else if ((incoming[0]='t') && (incoming[1]=='i') && (incoming[2]=='m') && (incoming[3]=='e'))
               toggletime = (toggletime+1)%2;

            // Rewind read cursor
            rcvcursor = 0;
         }

         if (checkchar != 8)
         {
            char shortstring[2];
            shortstring[0]=checkchar;
            shortstring[1]=0;
            EchoConsole(shortstring);
         }

         // Echo characters back to the terminal
         UARTTX[0] = checkchar;
         if (checkchar == 13)
            UARTTX[0] = 10; // Echo a linefeed

         if (rcvcursor>31)
            rcvcursor = 0;
      }

      if (gpustate == cnt) // GPU work complete, push more
      {
         ++cnt;
         // Show the char table
         ClearScreen(bgcolor);
         DrawConsole();

         // Cursor blink
         if (milliseconds-prevmilliseconds > 530)
         {
            prevmilliseconds = milliseconds;
            ++cursorevenodd;
         }

         // Cursor overlay
         if ((cursorevenodd%2) == 0)
         {
            int cx, cy;
            GetConsoleCursor(cx, cy);
            PrintDMA(cx*8, cy*8, "_");
         }

         if (toggletime)
         {
            uint32_t offst = PrintDMADecimal(0, 0, hours);
            PrintDMA(offst*8, 0, ":"); ++offst;
            offst += PrintDMADecimal(offst*8,0,minutes);
            PrintDMA(offst*8, 0, ":"); ++offst;
            offst += PrintDMADecimal(offst*8,0,seconds);
         }

         GPUFIFO[4] = GPUOPCODE(GPUVSYNC, 0, 0, 0);

         page = (page+1)%2;
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 6, GPU22BITIMM(page));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 6, 6, GPU10BITIMM(page));
         GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 6, 0, 0);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(&gpustate);
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(gpustateDWORDaligned));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(gpustateDWORDaligned));

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 2, GPU22BITIMM(cnt));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 2, 2, GPU10BITIMM(cnt));
         GPUFIFO[4] = GPUOPCODE(GPUSYSMEMOUT, 2, 1, 0);

         gpustate = 0;
      }
   }

   //free(incoming);

   return 0;
}
