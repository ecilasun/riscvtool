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
uint8_t *framebuffer[2];

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
      UARTWrite(FRtoString[fr]);
   }
}*/

int main()
{
   const unsigned char bgcolor = 0x1A; // Use default VGA palette grayscale value
   uint32_t vbcnt = 0xFF, prevvbcnt = 0xFF;
   char cmdbuffer[33*25];

   // Set output page
   uint32_t page = 0;

   framebuffer[0] = GPUAllocateBuffer(320*240);
   framebuffer[1] = GPUAllocateBuffer(320*240);

   GPUSetVPage((uint32_t)framebuffer[page]);
   GPUSetVMode(MAKEVMODEINFO(VIDEOMODE_320PALETTED, VIDEOOUT_ENABLED)); // Mode 1, video on

   uint8_t *writepage = framebuffer[(page+1)%2];

   // Startup message
   ClearConsole();
   GPUClearScreen(writepage, VIDEOMODE_320PALETTED, bgcolor);
   EchoConsole("MiniTerm (c)2021 Engin Cilasun\n Use 'help' for assistance\n");
   UARTWrite("MiniTerm (c)2021 Engin Cilasun\n");

   DrawConsole(writepage, FRAME_WIDTH_MODE0);

   // Flip and resume
   page = (page+1)%2;
   GPUSetVPage((uint32_t)writepage);

   // UART communication section
   uint32_t prevmilliseconds = 0;
   uint32_t cursorevenodd = 0;

   int escapemode = 0;
   while(1)
   {
      uint64_t clk = E32ReadTime();
      uint32_t milliseconds = ClockToMs(clk);
      uint32_t hours, minutes, seconds;
      ClockMsToHMS(milliseconds, &hours, &minutes, &seconds);

      if (UARTHasData())
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
               EchoConsole("\n");
               EchoConsole("\nMiniTerm version 0.1\n(c)2021 Engin Cilasun\ncls: clear screen\nhelp: help screen\nrun:branch to ELF\ndump:dump first 256 bytes of ELF\n");
               UARTWrite("\n");
               UARTWrite("\nMiniTerm version 0.1\n(c)2021 Engin Cilasun\ncls: clear screen\nhelp: help screen\nrun:branch to ELF\ndump:dump first 256 bytes of ELF\n");
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
            {
                for (uint32_t i=0x0A000000;i<0x0A000100;i+=4)
                {
                    UARTWriteHex(i);
                    UARTWrite(":");
                    UARTWriteHex(*((uint32_t*)i));
                    UARTWrite("\n");
                }
            }
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
               GetConsoleCursor(&cx, &cy);
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

      // Check to see if we've passed over the previous vblank counter
      vbcnt = GPUReadVBlankCounter();

      // Update and swap page only if we have
      if (vbcnt != prevvbcnt)
      {
         prevvbcnt = vbcnt;

         writepage = framebuffer[(page+1)%2];

         // Show the char table
         GPUClearScreen(writepage, VIDEOMODE_320PALETTED, bgcolor);
         DrawConsole(writepage, FRAME_WIDTH_MODE0);

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
            GetConsoleCursor(&cx, &cy);
            GPUPrintString(writepage, FRAME_WIDTH_MODE0, cx*8, cy*8, "_", 1);
         }

         // Swap
         page = (page+1)%2;
         GPUSetVPage((uint32_t)writepage);
      }
   }

   return 0;
}
