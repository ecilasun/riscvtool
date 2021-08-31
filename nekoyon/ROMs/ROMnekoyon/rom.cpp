// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <encoding.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"
#include "elf.h"

#include "fat32/ff.h"

FATFS Fs;

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

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   uint32_t at = read_csr(mtval);      // PC
   uint32_t cause = read_csr(mcause);  // Exception cause

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0)
   {
      uint32_t opcode = read_csr(mscratch);  // Instruction causing the exception

      // Show the address and the failing instruction's opcode field
      UARTWrite("EXCEPTION: Illegal instruction I$(0x");
      UARTWriteHex((uint32_t)opcode);
      UARTWrite(") D$(0x");
      UARTWriteHex(*(uint32_t*)at);
      UARTWrite(") at 0x");
      UARTWriteHex((uint32_t)at);
      UARTWrite("\n");
   }
   else
   {
      // We've hit a breakpoint
      // TODO: Tie this into GDB routines (connected via UART)
      UARTWrite("EXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
   }

   // Deadlock in either case for now
   while(1) { }
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
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      if (*IO_UARTRXByteAvailable)
         target[writecursor++] = *IO_UARTRXTX;
   }
}

typedef int (*t_mainfunction)();

void LaunchELF(uint32_t branchaddress)
{
   // TODO: Set up return environment before we branch into this routine

   // Branch to loaded ELF's entry point
   ((t_mainfunction)branchaddress)();

   // Done, back in our world
   UARTWrite("Run complete.\n");
}

void FlushDataCache()
{
   // Force D$ flush so that contents are visible by I$
   // We do this by forcing a dummy load of DWORDs from 0 to 2048
   // to force previous contents to be written back to DDR3
   for (uint32_t i=0; i<2048; ++i)
   {
      uint32_t dummyread = DDR3Start[i];
      // This is to make sure compiler doesn't eat our reads
      // and shuts up about unused variable
      asm volatile ("add x0, x0, %0;" : : "r" (dummyread) : );
   }
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

   // Force D$ to write contents back to DDR3
   // so that I$ can load them.
   FlushDataCache();

   LaunchELF(branchaddress);
}

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &branchaddress)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       UARTWrite(" failed: expecting 0x7F+'ELF'");
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
         UARTWrite(".");
         f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
      }
   }

   free(names);
}

int LoadAndRunELF(const char *fname)
{
   FIL fp;
   FRESULT fr = f_open(&fp, fname, FA_READ);
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
      LaunchELF(branchaddress);
      return 0;
   }
   else
   {
      UARTWrite("boot.elf not found\n");
      return -1;
   }
}

int main()
{
   InstallIllegalInstructionHandler();

   // Show startup info
   UARTWrite("\033[2J\n+-------------------------+\n|          ************** |\n| ########   ************ |\n| #########  ************ |\n| ########   ***********  |\n| #        ***********    |\n| ##   *************   ## |\n| ####   *********   #### |\n| ######   *****   ###### |\n| ########   *   ######## |\n| ##########   ########## |\n+-------------------------+\n");
   UARTWrite("\nNekoYon boot loader version N4:0002\nSingle core RISC-V @150Mhz\narch: RV32IZicsr\nDevices: UART,DDR3,SPI\n\u00A9 2021 Engin Cilasun\n\n");

   // Step A: BOOT phase

   // Step A1: Try to mount the SDCard
   uint32_t sdcardavailable = 0;
   FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
   if (mountattempt!=FR_OK)
      UARTWrite(FRtoString[mountattempt]);
   else
      sdcardavailable = 1;

   if (sdcardavailable)
   {
      // Step A2: Check to see if we can find a 'boot.elf' in root directory and run it
      LoadAndRunELF("sd:boot.elf");
   }
   else
   {
      // Step B: UART Phase, fallback when no boot.elf is found

      uint8_t prevchar = 0xFF;
      while(1)
      {
         // Step B1: Check to see if we have something in the FIFO
         if (*IO_UARTRXByteAvailable)
         {
            // Step B2: Read the data on UARTRX memory location
            char checkchar = *IO_UARTRXTX;

            // Step B3: Proceed to load binary blobs or run the incoming ELF
            // if we received a 'B\n' or 'R\n' sequence
            if (checkchar == 13)
            {
               // Load the incoming binary
               if (prevchar=='B')
                  LoadBinaryBlob();
               if (prevchar=='R')
                  RunBinaryBlob();
            }

            // Step B4: Remember this character for the next round
            prevchar = checkchar;
         }
      }
   }

   return 0;
}
