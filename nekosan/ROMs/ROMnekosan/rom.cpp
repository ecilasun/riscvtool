// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include <encoding.h>

#include "rvcrt0.h"

#include "core.h"
#include "audio.h"
#include "uart.h"
#include "leds.h"
#include "switches.h"
#include "gpu.h"

#include "elf.h"
#include "fat32/ff.h"

const char *rootpath = "sd:";

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   uint32_t at = read_csr(mtval);
   uint32_t cause = read_csr(mcause);

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0)
   {
      // Offending instruction's opcode field
      uint32_t opcode = read_csr(mscratch);

      // Show the address and the failing instruction's opcode field
      EchoStr("EXCEPTION: Illegal instruction I$(0x");
      EchoHex((uint32_t)opcode);
      EchoStr(") D$(0x");
      EchoHex(*(uint32_t*)at);
      EchoStr(") at 0x");
      EchoHex((uint32_t)at);
      EchoStr("\n");
   }
   else
   {
      // We've hit a breakpoint
      EchoStr("EXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
   }

   // Deadlock
   while(1) { }
}

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
char *executables[128];

FATFS Fs;

uint32_t vramPage = 0;

void CLS()
{
   // Clear all of the VRAM area (all 208 lines)
   GPUSetRegister(0, 0x01010101);
   GPUClearVideoPage(0);
}

void FLIP()
{
   // Selecting vramPage 0 will set scanoutPage to 1
   // Selecting vramPage 1 will set scanoutPage to 0
   GPUSetRegister(0, vramPage^0x1);
   GPUSetVideoPage(0);
   vramPage++;
}

void PostLoadCLS()
{
   // Restore VGA palette and clear both frame buffers

   ResetVGAPalette();

   // Erase both pages to black

   GPUSetRegister(0, 0x00000000);
   GPUClearVideoPage(0);
   FLIP();

   GPUSetRegister(0, 0x00000000);
   GPUClearVideoPage(0);
   FLIP();
}

void ShowSplashOrDirectoryListing()
{
   CLS();

   if (numexecutables == 0)
   {
      PrintDMA(132, 92, "NekoSan", false);
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

   FLIP();
}

void ShowLoadingScreen()
{
   CLS();

   PrintDMA(120, 92, "Loading...", false);

   FLIP();
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
      if (*IO_UARTRXByteAvailable)
         loadtargetaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      if (*IO_UARTRXByteAvailable)
         loadlenaschar[writecursor++] = *IO_UARTRXTX;
   }

   // Read binary blob
   writecursor = 0;
   volatile uint8_t* target = (volatile uint8_t* )loadtarget;
   while(writecursor < loadlen)
   {
      if (*IO_UARTRXByteAvailable)
      {
         target[writecursor++] = *IO_UARTRXTX;
         *IO_LEDRW = (writecursor>>9)&0x1; // Blink rightmost LED as status indicator
      }
   }
   *IO_LEDRW = 0x0; // Turn all LEDs off
}

typedef int (*t_mainfunction)();

void LaunchELF(uint32_t branchaddress)
{
   PostLoadCLS();

   // Branch to loaded executable's entry point
   ((t_mainfunction)branchaddress)();
   /*asm (
      "lw x31, %0 \n"
      "jalr ra, 0(x31) \n"
      : 
      : "m" (branchaddress)
      : 
   );*/

   // NOTE: Apps don't simply 'return' to caller, but rather
   // use a syscall to exit, so we might never reach here
   // unless the exit method is changed.

   // Done, back in our world
   EchoStr("Run complete.\n");
   ShowSplashOrDirectoryListing();
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
      uint32_t byteavailable = *IO_UARTRXByteAvailable;
      if (byteavailable != 0)
      {
         unsigned char readdata = *IO_UARTRXTX;
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   EchoStr("\nStarting @0x");
   EchoHex(branchaddress);
   EchoStr("\n");

   LaunchELF(branchaddress);

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &branchaddress)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       EchoStr(" failed: expecting 0x7F+'ELF'");
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
         EchoStr(".");
         f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
      }
   }

   free(names);
}

int LoadAndRunELF(int selection)
{
   FIL fp;
   char fullpath[255];
   strcpy(fullpath, rootpath);
   strcat(fullpath, executables[selection]);
   FRESULT fr = f_open(&fp, fullpath, FA_READ);
   if (fr == FR_OK)
   {
      ShowLoadingScreen();

      // File size: Fs.fsize
      // Read header
      SElfFileHeader32 fheader;
      UINT readsize;
      f_read(&fp, &fheader, sizeof(fheader), &readsize);
      uint32_t branchaddress;
      ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
      f_close(&fp);
      EchoStr("\nStarting ");
      EchoStr(executables[selection]);
      EchoStr("\n");
      LaunchELF(branchaddress);
      return 0;
   }
   else
   {
      EchoStr(FRtoString[fr]);
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
               strcpy(executables[numexecutables], finf.fname);
               executables[numexecutables][olen] = 0;
               ++numexecutables;
               //printf("%s %d %s\n", finf.fname, (int)finf.fsize, flags);
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      EchoStr(FRtoString[re]);
}

void InstallIllegalInstructionHandler()
{
   // Set machine software interrupt handler
   swap_csr(mtvec, illegal_instruction_exception);

   // Enable machine software interrupt (breakpoint/illegal instruction)
   swap_csr(mie, MIP_MSIP);

   // Enable machine interrupts
   swap_csr(mstatus, MSTATUS_MIE);
}

int main()
{
   // Illegal instruction trap
   InstallIllegalInstructionHandler();

   // Write silence to audio output
   *IO_AudioFIFO = 0x0;

   // Turn off all LEDs
   *IO_LEDRW = 0x0;

   // Set current 'write' video page (by setting current scan-out video page)
   FLIP();

   // Read switch state at startup
   hardwareswitchstates = oldhardwareswitchstates = *IO_SWITCHES;

   // Show startup info
   EchoStr("\033[2J\n");
   EchoStr("+-------------------------+\n");
   EchoStr("|          ************** |\n");
   EchoStr("| ########   ************ |\n");
   EchoStr("| #########  ************ |\n");
   EchoStr("| ########   ***********  |\n");
   EchoStr("| #        ***********    |\n");
   EchoStr("| ##   *************   ## |\n");
   EchoStr("| ####   *********   #### |\n");
   EchoStr("| ######   *****   ###### |\n");
   EchoStr("| ########   *   ######## |\n");
   EchoStr("| ##########   ########## |\n");
   EchoStr("+-------------------------+\n");
   EchoStr("\nNekoSan version 0001\n");
   EchoStr("rv32imfZicsr\n");
   EchoStr("Devices: UART/SPI/SWITCH/LED/AUDIO/GPU\n");
   EchoStr("\u00A9 2021 Engin Cilasun\n\n");

   uint32_t sdcardavailable = 0;
   FRESULT mountattempt = f_mount(&Fs, rootpath, 1);
   if (mountattempt!=FR_OK)
      EchoStr(FRtoString[mountattempt]);
   else
      sdcardavailable = 1;

   // Set color palette entries used by ROM

   GPUSetRegister(1, MAKERGBPALETTECOLOR(255, 255, 255));
   GPUSetRegister(0, 255); // index 255: text color (white)
   GPUSetPaletteEntry(0, 1);

   GPUSetRegister(1, MAKERGBPALETTECOLOR(128, 128, 255));
   GPUSetRegister(0, 0); // index 0: selection color (light blue)
   GPUSetPaletteEntry(0, 1);

   GPUSetRegister(1, MAKERGBPALETTECOLOR(128, 128, 128));
   GPUSetRegister(0, 1); // index 1: background color (gray)
   GPUSetPaletteEntry(0, 1);

   GPUSetRegister(1, MAKERGBPALETTECOLOR(32, 32, 32));
   GPUSetRegister(0, 2); // index 2: status bar color (dark gray)
   GPUSetPaletteEntry(0, 1);

   // UART communication section
   uint8_t prevchar = 0xFF;

   uint32_t dirListed = 0;

   InitFont();

   // Draw initial frame with NekoSan logo
   ShowSplashOrDirectoryListing();

   while(1)
   {
      // Step 1: Check to see if we have something in the FIFO
      if (*IO_UARTRXByteAvailable)
      {
         // Step 2: Read the data on UARTRX memory location
         char checkchar = *IO_UARTRXTX;

         // Step 3: Proceed to load binary blobs or run the incoming ELF
         // if we received a 'B\n' or 'R\n' sequence
         if (checkchar == 13)
         {
            ShowLoadingScreen();

            // Load the incoming binary
            if (prevchar=='B')
               LoadBinaryBlob();
            if (prevchar=='R')
               RunBinaryBlob();
         }

         // Step 4: Remember this character for the next round
         prevchar = checkchar;
      }

      // If we haven't listed the directory yet and if we have an SDCard,
      // list the directory into an internal buffer
      if (sdcardavailable && !dirListed)
      {
         ListDir(rootpath);
         dirListed = 1;
         // Show the directory listing
         ShowSplashOrDirectoryListing();
      }

      // Handle keys for when we do have a directory listing
      if (dirListed)
      {
         while (*IO_SWITCHREADY)
            hardwareswitchstates = *IO_SWITCHES;
         
         // Capture button states
         int down = 0, up = 0, sel = 0;
         switch(hardwareswitchstates ^ oldhardwareswitchstates)
         {
            case 0x01: sel = hardwareswitchstates&0x01 ? 0 : 1; break;
            case 0x02: down = hardwareswitchstates&0x02 ? 0 : 1; break;
            case 0x04: up = hardwareswitchstates&0x04 ? 0 : 1; break;
            default: break;
         };

         oldhardwareswitchstates = hardwareswitchstates;

         if (up) selectedexecutable--;
         if (down) selectedexecutable++;
         if (selectedexecutable>=numexecutables) selectedexecutable = 0;
         if (selectedexecutable<0) selectedexecutable = numexecutables-1;

         // Update selection display
         if (up || down || sel)
            ShowSplashOrDirectoryListing();

         if (sel) LoadAndRunELF(selectedexecutable);
      }
   }

   return 0;
}
