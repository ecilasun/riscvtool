// MiniTerm

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "nekoichi.h"
#include "gpu.h"
#include "sdcard.h"
#include "fat.h"
#include "console.h"
#include "elf.h"

FATFS Fs;
volatile uint32_t branchaddress = 0x10000;
const char *FRtoString[]={
   "FR_OK\r\n",
	"FR_DISK_ERR\r\n",
	"FR_NOT_READY\r\n",
	"FR_NO_FILE\r\n",
	"FR_NO_PATH\r\n",
	"FR_NOT_OPENED\r\n",
	"FR_NOT_ENABLED\r\n",
	"FR_NO_FILESYSTEM\r\n"
};

void parseelfheader(int fh, SElfFileHeader32 *fheader)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       EchoConsole("Failed: expecting 0x7F+'ELF'\n");
       return;
   }

   branchaddress = fheader->m_Entry;
   EchoConsole("Program entry point: ");
   EchoConsoleHex(branchaddress);
   EchoConsole("\r\n");

   WORD bytesread;

   // Read program header
   SElfProgramHeader32 pheader;
   pf_lseek(fh, fheader->m_PHOff);
   pf_read(fh, &pheader, sizeof(SElfProgramHeader32), &bytesread);

   // Read string table section header
   unsigned int stringtableindex = fheader->m_SHStrndx;
   SElfSectionHeader32 stringtablesection;
   pf_lseek(fh, fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
   pf_read(fh, &stringtablesection, sizeof(SElfSectionHeader32), &bytesread);

   // Allocate memory for and read string table
   char *names = (char *)malloc(stringtablesection.m_Size);
   pf_lseek(fh, stringtablesection.m_Offset);
   pf_read(fh, names, stringtablesection.m_Size, &bytesread);

   // Dump all section names and info
   for(unsigned short i=0; i<fheader->m_SHNum; ++i)
   {
      // Seek-load section headers as needed
      SElfSectionHeader32 sheader;
      pf_lseek(fh, fheader->m_SHOff+fheader->m_SHEntSize*i);
      pf_read(fh, &sheader, sizeof(SElfSectionHeader32), &bytesread);

      // If this is a section worth loading...
      if (sheader.m_Flags & 0x00000007 && sheader.m_Size!=0)
      {
         // TODO: Load this section to memory
         /*uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
         pf_lseek(sheader.m_Offset);
         pf_read(elfsectionpointer, sheader.m_Size, &bytesread);*/

         // DEBUG: dump info about sections to load
         EchoConsole(&names[sheader.m_NameOffset]);
         EchoConsole("\r\n");
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Addr);
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Size);
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Offset);
         EchoConsole("\r\n");
      }
   }

   free(names);
}

void loadelf(char *commandline)
{
   int fh = pf_open(&commandline[5]);
   if (fh != -1)
   {
      // File size: Fs.fsize
      WORD bytesread;
      // Read header
      SElfFileHeader32 fheader;
      pf_read(fh, &fheader, sizeof(fheader), &bytesread);
      parseelfheader(fh, &fheader);
      pf_close(fh);
   }
   else
   {
      EchoConsole("\r\nNot found:");
      EchoConsole(&commandline[5]);
   }
}

void showdir()
{
   DIR dir;
   FRESULT re = pf_opendir(&dir, " ");
   if (re == FR_OK)
   {
      FILINFO finf;
      do{
         re = pf_readdir(&dir, &finf);
         if (re == FR_OK && dir.sect!=0)
         {
            EchoConsole(finf.fname);
            EchoConsole(" ");
            EchoConsole(finf.fsize);
            EchoConsole(" ");
            EchoUART(finf.fname);
            EchoUART(" ");
            EchoHex(finf.fsize);
            EchoUART(" ");
            /*EchoConsole(1944 + ((finf.ftime&0xFE00)>>9));
            EchoConsole("/");
            EchoConsole((finf.ftime&0x1E0)>>5);
            EchoConsole("/");
            EchoConsole(finf.ftime&0x1F);*/
            if (finf.fattrib&0x01) { EchoConsole("r"); EchoUART("r"); }
            if (finf.fattrib&0x02) { EchoConsole("h"); EchoUART("h"); }
            if (finf.fattrib&0x04) { EchoConsole("s"); EchoUART("s"); }
            if (finf.fattrib&0x08) { EchoConsole("l"); EchoUART("l"); }
            if (finf.fattrib&0x0F) { EchoConsole("L"); EchoUART("L"); }
            if (finf.fattrib&0x10) { EchoConsole("d"); EchoUART("d"); }
            if (finf.fattrib&0x20) { EchoConsole("a"); EchoUART("a"); }

            EchoConsole("\r\n");
            EchoUART("\r\n");
         }
      } while(re == FR_OK && dir.sect!=0);
   }
   else
      EchoUART(FRtoString[re]);

}

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
               EchoConsole("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ndir: list files\r\nload filename: load and run ELF\n\rcls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\nrun:branch to ELF\r\ndump:dump first 256 bytes of ELF\r\n");
               EchoUART("\r\n");
               EchoUART("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ndir: list files\r\nload filename: load and run ELF\n\rcls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\nrun:branch to ELF\r\ndump:dump first 256 bytes of ELF\r\n");
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='r'))
            {
               EchoConsole("\r\n");
               showdir();
            }
            else if ((cmdbuffer[0]='l') && (cmdbuffer[1]=='o') && (cmdbuffer[2]=='a') && (cmdbuffer[3]=='d'))
            {
               EchoConsole("\r\n");
               loadelf(cmdbuffer);
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
            {
                for (uint32_t i=branchaddress;i<branchaddress + 0x100;i+=4)
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
                  "li x12, 0x0001FFF0 \n"
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
