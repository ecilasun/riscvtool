// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <ctype.h>

#include "rvcrt0.h"

#include "../nekoSDK/nekoichi.h"
#include "../nekoSDK/gpu.h"
#include "../nekoSDK/uart.h"
#include "../nekoSDK/elf.h"
#include "../nekoSDK/switches.h"
#include "../3rdparty/fat32/ff.h"

const char *FRtoString[]={
	"Succeeded\n",
	"A hard error occurred in the low level disk I/O layer\n",
	"Assertion failed\n",
	"The physical drive cannot work\n",
	"Could not find the file\n",
	"Could not find the path\n",
	"The path name format is invalid\n",
	"Access denied due to prohibited access or directory full\n",
	"Access denied due to prohibited access\n",
	"The file/directory object is invalid\n",
	"The physical drive is write protected\n",
	"The logical drive number is invalid\n",
	"The volume has no work area\n",
	"There is no valid FAT volume\n",
	"The f_mkfs() aborted due to any problem\n",
	"Could not get a grant to access the volume within defined period\n",
	"The operation is rejected according to the file sharing policy\n",
	"LFN working buffer could not be allocated\n",
	"Number of open files > FF_FS_LOCK\n",
	"Given parameter is invalid\n"
};

uint32_t hardwareswitchstates, oldhardwareswitchstates;
int selectedexecutable = 0;
int numexecutables = 0;
char *executables[64];

FATFS Fs;
volatile uint32_t *gpuSideSubmitCounter = (volatile uint32_t *)(GraphicsRAMEnd-16); // Lives one DWORD before the stack
uint32_t gpuSubmitCounter = 0;
uint32_t vramPage = 0;

void SubmitGPUFrame()
{
   // Do not submit more work if the GPU is not ready
   if (*gpuSideSubmitCounter == gpuSubmitCounter)
   {
      // Next frame
      ++gpuSubmitCounter;

      // CLS
      GPUSetRegister(1, 0x16161616); // 4 dark gray pixels
      GPUClearVRAMPage(1);

      if (numexecutables == 0)
      {
         PrintDMA(96, 92, "NEKOICHI", false);
      }
      else
      {
         int line = 0;
         for (int i=0;i<numexecutables;++i)
         {
            PrintDMA(8, line, executables[i], (selectedexecutable == i) ? false : true);
            line += 8;
         }
      }

      // Stall GPU until vsync is reached
      GPUWaitForVsync();

      // Swap to other page to reveal previous render
      vramPage = (vramPage+1)%2;
      GPUSetRegister(2, vramPage);
      GPUSetVideoPage(2);

      // GPU will write value in G2 to address in G3 in the future
      GPUSetRegister(3, uint32_t(gpuSideSubmitCounter));
      GPUSetRegister(2, gpuSubmitCounter);
      GPUWriteToGraphicsMemory(2, 3);

      // Clear state, GPU will overwrite this when it reaches GPUSYSMEMOUT
      *gpuSideSubmitCounter = 0;
   }
}

void SubmitBlankFrameNoWait()
{
   GPUSetRegister(1, 0x10101010);
   GPUClearVRAMPage(1);
   // Stall GPU until vsync is reached
   GPUWaitForVsync();

   // Swap to other page to reveal previous render
   vramPage = (vramPage+1)%2;
   GPUSetRegister(2, vramPage);
   GPUSetVideoPage(2);
}

void LoadBinaryBlob()
{
   // Header data
   uint32_t loadlen = 0;
   uint32_t loadtarget = 0;
   char *loadlenaschar = (char*)&loadlen;
   char *loadtargetaschar = (char*)&loadtarget;

   // Target address
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         loadlenaschar[writecursor++] = readdata;
      }
   }

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         target[writecursor++] = readdata;
      }
   }
}

void LaunchELF(uint32_t branchaddress)
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
      "li x12, 0x0FFFF000 \n" // we're not coming back to the loader, set SP to near the end of RAM
      "mv sp, x12 \n"
      "ret \n"
      : 
      : "m" (branchaddress)
      : 
   );
}

void RunBinaryBlob()
{
   // Header data
   uint32_t branchaddress = 0;
   char *branchaddressaschar = (char*)&branchaddress;

   // Data length
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   EchoUART("'remote.elf' starting\n");
   LaunchELF(branchaddress);

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &branchaddress)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       EchoUART(" failed: expecting 0x7F+'ELF'");
       return;
   }

   branchaddress = fheader->m_Entry;

   // Read program header
   SElfProgramHeader32 pheader;
   f_lseek(fp, fheader->m_PHOff);
   UINT bytesread;
   f_read(fp, &pheader, sizeof(SElfProgramHeader32), &bytesread);

   // Read string table section header
   unsigned int stringtableindex = fheader->m_SHStrndx;
   SElfSectionHeader32 stringtablesection;
   f_lseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
   f_read(fp, &stringtablesection, sizeof(SElfSectionHeader32), &bytesread);

   // Allocate memory for and read string table
   char *names = (char *)malloc(stringtablesection.m_Size);
   f_lseek(fp, stringtablesection.m_Offset);
   f_read(fp, names, stringtablesection.m_Size, &bytesread);

   // Load all loadable sections
   for(unsigned short i=0; i<fheader->m_SHNum; ++i)
   {
      // Seek-load section headers as needed
      SElfSectionHeader32 sheader;
      f_lseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*i);
      f_read(fp, &sheader, sizeof(SElfSectionHeader32), &bytesread);

      // If this is a section worth loading...
      if (sheader.m_Flags & 0x00000007 && sheader.m_Size!=0)
      {
         // ...place it in memory
         uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
         f_lseek(fp, sheader.m_Offset);
         EchoUART(".");
         f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
      }
   }

   free(names);
}

int LoadAndRunELF(int selection)
{
   FIL fp;
   FRESULT fr = f_open(&fp, executables[selection], FA_READ);
   if (fr == FR_OK)
   {
      // File size: Fs.fsize
      // Read header
      SElfFileHeader32 fheader;
      UINT readsize;
      f_read(&fp, &fheader, sizeof(fheader), &readsize);
      uint32_t branchaddress;
      ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
      f_close(&fp);
      EchoUART("starting\n");
      LaunchELF(branchaddress);
      return 0;
   }
   else
   {
      EchoUART(FRtoString[fr]);
      return -1;
   }
}

void ListDir(const char *path)
{
   numexecutables = 0;
   DIR dir;
   FRESULT re = f_opendir(&dir, path);
   if (re == FR_OK)
   {
      FILINFO finf;
      do{
         re = f_readdir(&dir, &finf);
         if (re == FR_OK && dir.sect!=0)
         {
            // We're only interested in executables
            if (strstr(finf.fname,".elf"))
            {
               /*int fidx=0;
               char flags[64]="";
               if (finf.fattrib&0x01) flags[fidx++]='r';
               if (finf.fattrib&0x02) flags[fidx++]='h';
               if (finf.fattrib&0x04) flags[fidx++]='s';
               if (finf.fattrib&0x08) flags[fidx++]='l';
               if (finf.fattrib&0x0F) flags[fidx++]='L';
               if (finf.fattrib&0x10) flags[fidx++]='d';
               if (finf.fattrib&0x20) flags[fidx++]='a';
               flags[fidx++]=0;*/
               int olen = strlen(finf.fname) + 3;
               executables[numexecutables] = (char*)malloc(olen+1);
               strcpy(executables[numexecutables], "sd:");
               strcat(executables[numexecutables], finf.fname);
               executables[numexecutables][olen] = 0;
               ++numexecutables;
               //printf("%s %d %s\r\n", finf.fname, (int)finf.fsize, flags);
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      EchoUART(FRtoString[re]);
}

int main()
{
   // Read switch state at startup
   hardwareswitchstates = oldhardwareswitchstates = *IO_SwitchState;

   InitFont();
   EchoUART("\nNekoIchi [0x000B] [RV32IMFZicsr]\n\u00A9 2021 Engin Cilasun\n");

   uint32_t sdcardavailable = 0;
   FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
   if (mountattempt!=FR_OK)
      EchoUART(FRtoString[mountattempt]);
   else
      sdcardavailable = 1;

   // Initialize video page
   GPUSetRegister(2, vramPage);
   GPUSetVideoPage(2);
   *gpuSideSubmitCounter = 0;

   // Make text color white and text background color light blue
   uint32_t colorwhite = MAKERGBPALETTECOLOR(255, 255, 255);
   uint32_t colorblue = MAKERGBPALETTECOLOR(128, 128, 255);
   GPUSetRegister(1, colorwhite);
   GPUSetRegister(2, colorblue);
   GPUSetPaletteEntry(1, 255);
   GPUSetPaletteEntry(2, 0);

   // UART communication section
   uint8_t prevchar = 0xFF;

   uint64_t clk = ReadClock();
   uint32_t old_milliseconds = ClockToMs(clk);
   uint32_t dirListed = 0;

   while(1)
   {
      // Step 1: Read UART FIFO byte count
      uint32_t bytecount = *IO_UARTRXByteCount;

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = *IO_UARTRX;

         if (checkchar == 13) // Enter followed by B (binary blob) or R (run blob)
         {
            // Load the incoming binary
            if (prevchar=='B')
               LoadBinaryBlob();
            if (prevchar=='R')
               RunBinaryBlob();
         }
         prevchar = checkchar;
      }

      clk = ReadClock();
      uint32_t new_milliseconds = ClockToMs(clk);

      if (sdcardavailable && !dirListed && ((new_milliseconds-old_milliseconds) > 1500))
      {
         ListDir("sd:");
         dirListed = 1;
      }

      if (dirListed)
      {
         hardwareswitchstates = *IO_SwitchState;

         int down = 0, up = 0, sel = 0;
         switch(hardwareswitchstates ^ oldhardwareswitchstates)
         {
            case 0x10: sel = hardwareswitchstates&0x10 ? 0 : 1; break;
            case 0x20: down = hardwareswitchstates&0x20 ? 0 : 1; break;
            case 0x40: up = hardwareswitchstates&0x40 ? 0 : 1; break;
            default: break;
         };

         oldhardwareswitchstates = hardwareswitchstates;

         if (up) selectedexecutable--;
         if (down) selectedexecutable++;
         if (selectedexecutable>=numexecutables) selectedexecutable = 0;
         if (selectedexecutable<0) selectedexecutable = numexecutables-1;
         if (sel) { SubmitBlankFrameNoWait(); LoadAndRunELF(selectedexecutable); }
      }

      SubmitGPUFrame();
   }

   return 0;
}
