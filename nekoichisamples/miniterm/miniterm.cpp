// MiniTerm

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "core.h"
#include "gpu.h"
#include "uart.h"
#include "sdcard.h"
#include "console.h"
#include "elf.h"

volatile uint32_t branchaddress = 0x10000;

/*void loadfile(char *commandline)
{
   FIL fp;
   FRESULT fr = f_open(&fp, &commandline[5], FA_READ);
   if (fr == FR_OK)
   {
      uint32_t fsz = f_size(&fp);
      UINT bytesread;
      f_read(&fp, (void*)0x0A000000, fsz, &bytesread);;
      f_close(&fp);
   }
   else
   {
      EchoUART(FRtoString[fr]);
   }
}*/

int main()
{
   InitFont();

   const unsigned char bgcolor = 0x1A; // Use default VGA palette grayscale value

   char cmdbuffer[33*25];

   // Set text color - default is 255 (black)
   /*{
      int r=0x10;
      int g=0xFF;
      int b=0xFF;
      uint32_t color = (g<<16) | (r<<8) | b;
      GPUSetRegister(1, color);
      GPUSetPaletteEntry(1, 255);
   }*/

   // Set output page
   uint32_t page = 0;
   GPUSetRegister(6, page);
   GPUSetVideoPage(6);

   // Startup message
   ClearConsole();
   ClearScreen(bgcolor);
   EchoConsole("MiniTerm (c)2021 Engin Cilasun\r\n Use 'help' for assistance\r\n");
   EchoUART("MiniTerm (c)2021 Engin Cilasun\r\n");
   DrawConsole();

   page = (page+1)%2;
   GPUSetRegister(6, page);
   GPUSetVideoPage(6);

   // UART communication section
   uint32_t prevmilliseconds = 0;
   uint32_t cursorevenodd = 0;
   uint32_t toggletime = 0;

   // Make sure this lands in some unused area of the GraphicsRAM
   volatile uint32_t *gpustate = (volatile uint32_t *)GraphicsFontStart-8;
   *gpustate = 0x0;
   unsigned int cnt = 0x00000000;

   int escapemode = 0;
   while(1)
   {
      uint64_t clk = ReadClock();
      uint32_t milliseconds = ClockToMs(clk);
      uint32_t hours, minutes, seconds;
      ClockMsToHMS(milliseconds, hours,minutes,seconds);

      // Step 1: Read UART FIFO byte count
      unsigned int bytecount = *IO_UARTRXByteCount;

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on IO_UARTRX memory location
         char checkchar = *IO_UARTRX;

         if (checkchar == 8) // Backspace? (make sure your terminal uses ctrl+h for backspace)
         {
            ConsoleCursorStepBack();
            EchoConsole(" ");
            ConsoleCursorStepBack();
         }
         else if (checkchar == 27) // ESC
         {
            escapemode = 1;
         }
         else if (checkchar == 13) // Enter?
         {
            // First, copy current row
            ConsoleStringAtRow(cmdbuffer);
            // Next, send a newline to go down one
            EchoConsole("\n");

            // Clear the whole screen
            if ((cmdbuffer[0]='c') && (cmdbuffer[1]=='l') && (cmdbuffer[2]=='s'))
            {
               ClearConsole();
            }
            else if ((cmdbuffer[0]='h') && (cmdbuffer[1]=='e') && (cmdbuffer[2]=='l') && (cmdbuffer[3]=='p'))
            {
               EchoConsole("\r\n");
               EchoConsole("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ncls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\nrun:branch to ELF\r\ndump:dump first 256 bytes of ELF\r\n");
               EchoUART("\r\n");
               EchoUART("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ncls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\nrun:branch to ELF\r\ndump:dump first 256 bytes of ELF\r\n");
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
            {
                for (uint32_t i=0x0A000000;i<0x0A000100;i+=4)
                {
                    EchoHex(i);
                    EchoUART(":");
                    EchoHex(*((uint32_t*)i));
                    EchoUART("\r\n");
                }
            }
            else if ((cmdbuffer[0]='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
               toggletime = (toggletime+1)%2;
            else if ((cmdbuffer[0]='r') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='n'))
            {
               asm (
                  "lw ra, %0 \n"
                  "li x12, 0x0FFFF000 \n"
                  "mv sp, x12 \n"
                  "ret \n"
                  : 
                  : "m" (branchaddress)
                  : 
               );
            }
         }

         if (escapemode)
         {
            ++escapemode;
            if (escapemode==4)
            {
               int cx,cy;
               GetConsoleCursor(cx, cy);
               if (checkchar == 'A')
                  SetConsoleCursor(cx, cy-1);
               if (checkchar == 'B')
                  SetConsoleCursor(cx, cy+1);
               if (checkchar == 'C')
                  SetConsoleCursor(cx+1, cy);
               if (checkchar == 'D')
                  SetConsoleCursor(cx-1, cy);
               escapemode = 0;
            }
         }
         else if (checkchar != 8)
         {
            char shortstring[2];
            shortstring[0]=checkchar;
            shortstring[1]=0;
            EchoConsole(shortstring);
         }

         // Echo characters back to the terminal
         *IO_UARTTX = checkchar;
         if (checkchar == 13)
            *IO_UARTTX = 10; // Echo extra linefeed
      }

      if (*gpustate == cnt) // GPU work complete, push more
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

         GPUWaitForVsync();

         page = (page+1)%2;
         GPUSetRegister(6, page);
         GPUSetVideoPage(6);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteToGraphicsMemory(2, 1);

         *gpustate = 0;
      }
   }

   return 0;
}
