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
#include "elf.h"

//static int s_filesystemready = 0;
volatile unsigned int targetjumpaddress = 0x00000000;
FATFS Fs;

void parseelfheader(unsigned char *_elfbinary)
{
    // Parse the header and check the magic word
    SElfFileHeader32 *fheader = (SElfFileHeader32 *)_elfbinary;

    if (fheader->m_Magic != 0x464C457F)
   {
       EchoConsole("Failed: expecting 0x7F+'ELF'\n");
       return;
   }

   unsigned int stringtableindex = fheader->m_SHStrndx;
   SElfSectionHeader32 *stringtablesection = (SElfSectionHeader32 *)(_elfbinary+fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
   char *names = (char*)(_elfbinary+stringtablesection->m_Offset);

   for(int i=0; i<fheader->m_SHNum; ++i)
   {
      SElfSectionHeader32 *sheader = (SElfSectionHeader32 *)(_elfbinary+fheader->m_SHOff+fheader->m_SHEntSize*i);

      char sectionname[128];
      int n=0;
      do
      {
         sectionname[n] = names[sheader->m_NameOffset+n];
         ++n;
      }
      while(names[sheader->m_NameOffset+n]!='.' && sheader->m_NameOffset+n<stringtablesection->m_Size);
      sectionname[n] = 0;

      EchoConsole("Section ");
      EchoConsole(i);
      EchoConsole(" : '");
      EchoConsole(sectionname);//, (sheader->m_Addr-pheader->m_PAddr), sheader->m_Size, sheader->m_Offset);
      EchoConsole("'\n");
   }
}

void loadelf(char *commandline)
{
   FRESULT fr = pf_open(&commandline[5]);
   if (fr == FR_OK)
   {
      WORD bytesread;
      unsigned char *buffer = (unsigned char*)malloc(Fs.fsize);
      pf_read(buffer, Fs.fsize, &bytesread);
      parseelfheader((unsigned char*)buffer);
   }
   else
   {
      EchoConsole("\r\nNot found:");
      EchoConsole(&commandline[5]);
   }
}

const char *FRtoString[]={
   "FR_OK",
	"FR_DISK_ERR",
	"FR_NOT_READY",
	"FR_NO_FILE",
	"FR_NO_PATH",
	"FR_NOT_OPENED",
	"FR_NOT_ENABLED",
	"FR_NO_FILESYSTEM"
};

int main()
{
   pf_mount(&Fs);

   const unsigned char bgcolor = 0xC0; // BRG -> B=0xC0, R=0x38, G=0x07
   //const unsigned char editbgcolor = 0x00;

   char cmdbuffer[33*25];

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
   volatile unsigned int gpustate = 0x00000000;
   unsigned int cnt = 0x00000000;
   int escapemode = 0;
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
         char checkchar = UARTRX[0];

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
               EchoConsole("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ndir: list files\r\nload filename: load and run ELF\n\rcls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\n");
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='r'))
            {
               EchoConsole("\r\n");
               DIR dir;
               FRESULT re = pf_opendir(&dir, " ");
               if (re == FR_OK)
               {
                  FILINFO finf;
                  do{
                     re = pf_readdir(&dir, &finf);
                     if (re == FR_OK)
                     {
                        EchoConsole(finf.fname);
                        EchoConsole("\r\n");
                     }
                  } while(re == FR_OK && dir.sect!=0);
               }
               else
                  EchoUART(FRtoString[re]);
            }
            else if ((cmdbuffer[0]='l') && (cmdbuffer[1]=='o') && (cmdbuffer[2]=='a') && (cmdbuffer[3]=='d'))
            {
               loadelf(cmdbuffer);
            }
            else if ((cmdbuffer[0]='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
               toggletime = (toggletime+1)%2;
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
         UARTTX[0] = checkchar;
         if (checkchar == 13)
            UARTTX[0] = 10; // Echo extra linefeed
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

         GPUWaitForVsync();

         page = (page+1)%2;
         GPUSetRegister(6, page);
         GPUSetVideoPage(6);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(&gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteSystemMemory(2, 1);

         gpustate = 0;
      }
   }

   return 0;
}
