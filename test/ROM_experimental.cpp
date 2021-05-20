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

#define STARTUP_ROM
#include "rom_nekoichi_rvcrt0.h"

FATFS Fs;
uint32_t sdcardavailable = 0;
volatile uint32_t branchaddress = 0x10000; // TODO: Branch to actual entry point
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

void runelf()
{
    // Set up stack pointer and branch to loaded executable's entry point (noreturn)
    // TODO: Can we work out the stack pointer to match the loaded ELF's layout?
    asm (
        "lw ra, %0 \n"
        "fmv.w.x	f0, zero \n"
        "fmv.w.x	f1, zero \n"
        "fmv.w.x	f2, zero \n"
        "fmv.w.x	f3, zero \n"
        "fmv.w.x	f4, zero \n"
        "fmv.w.x	f5, zero \n"
        "fmv.w.x	f6, zero \n"
        "fmv.w.x	f7, zero \n"
        "fmv.w.x	f8, zero \n"
        "fmv.w.x	f9, zero \n"
        "fmv.w.x	f10, zero \n"
        "fmv.w.x	f11, zero \n"
        "fmv.w.x	f12, zero \n"
        "fmv.w.x	f13, zero \n"
        "fmv.w.x	f14, zero \n"
        "fmv.w.x	f15, zero \n"
        "fmv.w.x	f16, zero \n"
        "fmv.w.x	f17, zero \n"
        "fmv.w.x	f18, zero \n"
        "fmv.w.x	f19, zero \n"
        "fmv.w.x	f20, zero \n"
        "fmv.w.x	f21, zero \n"
        "fmv.w.x	f22, zero \n"
        "fmv.w.x	f23, zero \n"
        "fmv.w.x	f24, zero \n"
        "fmv.w.x	f25, zero \n"
        "fmv.w.x	f26, zero \n"
        "fmv.w.x	f27, zero \n"
        "fmv.w.x	f28, zero \n"
        "fmv.w.x	f29, zero \n"
        "fmv.w.x	f30, zero \n"
        "fmv.w.x	f31, zero \n"
        "li x12, 0x0001FFF0 \n"
        "mv sp, x12 \n"
        "ret \n"
        : 
        : "m" (branchaddress)
        : 
    );
}

void parseelfheader(SElfFileHeader32 *fheader)
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
   pf_lseek(fheader->m_PHOff);
   pf_read(&pheader, sizeof(SElfProgramHeader32), &bytesread);

   // Read string table section header
   unsigned int stringtableindex = fheader->m_SHStrndx;
   SElfSectionHeader32 stringtablesection;
   pf_lseek(fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
   pf_read(&stringtablesection, sizeof(SElfSectionHeader32), &bytesread);

   // Allocate memory for and read string table
   char *names = (char *)malloc(stringtablesection.m_Size);
   pf_lseek(stringtablesection.m_Offset);
   pf_read(names, stringtablesection.m_Size, &bytesread);

   // Dump all section names and info
   for(unsigned short i=0; i<fheader->m_SHNum; ++i)
   {
      // Seek-load section headers as needed
      SElfSectionHeader32 sheader;
      pf_lseek(fheader->m_SHOff+fheader->m_SHEntSize*i);
      pf_read(&sheader, sizeof(SElfSectionHeader32), &bytesread);

      // If this is a section worth loading...
      if (sheader.m_Flags & 0x00000007 && sheader.m_Size!=0)
      {
         // TODO: Load this section to memory
         uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
         pf_lseek(sheader.m_Offset);
         pf_read(elfsectionpointer, sheader.m_Size, &bytesread);

         // DEBUG: dump info about sections to load
         /*EchoConsole(&names[sheader.m_NameOffset]);
         EchoConsole("\r\n");
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Addr);
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Size);
         EchoConsole(" ");
         EchoConsoleHex(sheader.m_Offset);
         EchoConsole("\r\n");*/
      }
   }

   free(names);
}

int loadelf(const char *filename)
{
   FRESULT fr = pf_open(filename);
   if (fr == FR_OK)
   {
      // File size: Fs.fsize
      WORD bytesread;
      // Read header
      SElfFileHeader32 fheader;
      pf_read(&fheader, sizeof(fheader), &bytesread);
      parseelfheader(&fheader);
      return 0;
   }
   else
   {
      EchoConsole("\r\nNot found:");
      EchoConsole(filename);
      return -1;
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
            /*EchoConsole(1944 + ((finf.ftime&0xFE00)>>9));
            EchoConsole("/");
            EchoConsole((finf.ftime&0x1E0)>>5);
            EchoConsole("/");
            EchoConsole(finf.ftime&0x1F);*/
            if (finf.fattrib&0x01) EchoConsole("r");
            if (finf.fattrib&0x02) EchoConsole("h");
            if (finf.fattrib&0x04) EchoConsole("s");
            if (finf.fattrib&0x08) EchoConsole("l");
            if (finf.fattrib&0x0F) EchoConsole("L");
            if (finf.fattrib&0x10) EchoConsole("d");
            if (finf.fattrib&0x20) EchoConsole("a");
            EchoConsole("\r\n");
         }
      } while(re == FR_OK && dir.sect!=0);
   }
   else
      EchoUART(FRtoString[re]);

}

int main()
{
   EchoUART("              vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("                  vvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrr       vvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vvvvvvvvvvvvvvvvvvvvvv  \r\n");
   EchoUART("rrrrrrrrrrrrr       vvvvvvvvvvvvvvvvvvvvvv    \r\n");
   EchoUART("rr                vvvvvvvvvvvvvvvvvvvvvv      \r\n");
   EchoUART("rr            vvvvvvvvvvvvvvvvvvvvvvvv      rr\r\n");
   EchoUART("rrrr      vvvvvvvvvvvvvvvvvvvvvvvvvv      rrrr\r\n");
   EchoUART("rrrrrr      vvvvvvvvvvvvvvvvvvvvvv      rrrrrr\r\n");
   EchoUART("rrrrrrrr      vvvvvvvvvvvvvvvvvv      rrrrrrrr\r\n");
   EchoUART("rrrrrrrrrr      vvvvvvvvvvvvvv      rrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrr      vvvvvvvvvv      rrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrr      vvvvvv      rrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vv      rrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr          rrrrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrrrr      rrrrrrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrrrrrr  rrrrrrrrrrrrrrrrrrrrrr\r\n");

   EchoUART("\r\nNekoIchi [v003] [rv32imf] [GPU]\r\n");
   EchoUART("(c)2021 Engin Cilasun\r\n");

   sdcardavailable = (pf_mount(&Fs) == FR_OK) ? 1 : 0;

   // Load and branch into the boot executable if one is found on the SDCard
   if (loadelf("BOOT.ELF") != -1)
       runelf();

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
         // Step 3: Read the data on UARTRX memory location
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
               EchoConsole("\r\nMiniTerm version 0.1\r\n(c)2021 Engin Cilasun\r\ndir: list files\r\nload filename: load and run ELF\n\rcls: clear screen\r\nhelp: help screen\r\ntime: elapsed time\r\nrun:branch to entrypoint\r\ndump:dump first 256 bytes of ELF\r\n");
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='r'))
            {
               EchoConsole("\r\n");
               showdir();
            }
            else if ((cmdbuffer[0]='l') && (cmdbuffer[1]=='o') && (cmdbuffer[2]=='a') && (cmdbuffer[3]=='d'))
            {
               EchoConsole("\r\n");
               loadelf(&cmdbuffer[5]); // Skip 'load ' part
            }
            else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
            {
                for (uint32_t i=branchaddress;i<branchaddress + 0x100;i+=4)
                {
                    EchoInt(i);
                    EchoUART(":");
                    EchoInt(*((uint32_t*)i));
                    EchoUART("\r\n");
                }
            }
            else if ((cmdbuffer[0]='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
               toggletime = (toggletime+1)%2;
            else if ((cmdbuffer[0]='r') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='n'))
                runelf();
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
