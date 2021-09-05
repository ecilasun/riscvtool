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
uint32_t branchaddress = 0;
uint32_t savedstackpointer = 0;

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

typedef int (*t_mainfunction)();

void __attribute__((aligned(256))) __attribute__((naked)) LaunchELF(uint32_t jumptarget)
{
   asm volatile(
      // Save return address and s0
      "addi sp,sp,-16;"
      "sw ra,12(sp);"
      "sw s0,8(sp);"

      // Load jumptarget into s0
      "mv s0,a0;"

      // Call flush data cache
      "jal ra, _Z14FlushDataCachev;"

      // Restore the return address and s0 as if we're returning from LaunchELF()
      "lw ra,12(sp);"
      //"lw s0,8(sp);"
      "addi sp,sp,16;"

      // Save all registers
      "sw sp, %0;"
      "addi	sp,sp,-128;"
      "sw	 x1,120(sp);"  // ra -> this is the place where launchelf was called from
      "sw	 x2,116(sp);"  // sp
      "sw	 x3,112(sp);"  // gp
      "sw	 x4,108(sp);"  // tp
      "sw	 x5,104(sp);"  // t0
      "sw	 x6,100(sp);"  // t1
      "sw	 x7, 96(sp);"  // t2
      "sw	 x8, 92(sp);"  // s0
      "sw	 x9, 88(sp);"  // s1
      "sw	x10, 84(sp);"  // a0
      "sw	x11, 80(sp);"  // a1
      "sw	x12, 76(sp);"  // a2
      "sw	x13, 72(sp);"  // a3
      "sw	x14, 68(sp);"  // a4
      "sw	x15, 64(sp);"  // a5
      "sw	x16, 60(sp);"  // a6
      "sw	x17, 56(sp);"  // a7
      "sw	x18, 52(sp);"  // s2
      "sw	x19, 48(sp);"  // s3
      "sw	x20, 44(sp);"  // s4
      "sw	x21, 40(sp);"  // s5
      "sw	x22, 36(sp);"  // s6
      "sw	x23, 32(sp);"  // s7
      "sw	x24, 28(sp);"  // s8
      "sw	x25, 24(sp);"  // s9
      "sw	x26, 20(sp);"  // s1
      "sw	x27, 16(sp);"  // s1
      "sw	x28, 12(sp);"  // t3
      "sw	x29,  8(sp);"  // t4
      "sw	x30,  4(sp);"  // t5
      "sw	x31,  0(sp);"  // t6

      // Call function at jumptarget
      // Note that the ELF will not return back here but will do an ecall 93
      // and check a0 return value, and if it's zero or greater, will go into
      // an infinite loop (or if less than zero, throw error and then go to loop)
      // We will resolve this by recovering our state through an interrupt handler.
      "jalr s0;" : "=m" (savedstackpointer) : :
   );
}

void __attribute__((aligned(256))) __attribute__((naked)) ReturnFromELF()
{
      // NOTE: It is not possible for an ordinary gcc ELF to return
      // here without a custom _start/_exit pair.
   asm volatile(
      // Restore all registers
      "lw sp, %0;"
      "addi	sp,sp,-128;"   // land exactly where the SP was before
      "lw	 x1,120(sp);"  // ra -> LaunchELF() return address
      "lw	 x2,116(sp);"  // sp
      "lw	 x3,112(sp);"  // gp
      "lw	 x4,108(sp);"  // tp
      "lw	 x5,104(sp);"  // t0
      "lw	 x6,100(sp);"  // t1
      "lw	 x7, 96(sp);"  // t2
      "lw	 x8, 92(sp);"  // s0
      "lw	 x9, 88(sp);"  // s1
      "lw	x10, 84(sp);"  // a0
      "lw	x11, 80(sp);"  // a1
      "lw	x12, 76(sp);"  // a2
      "lw	x13, 72(sp);"  // a3
      "lw	x14, 68(sp);"  // a4
      "lw	x15, 64(sp);"  // a5
      "lw	x16, 60(sp);"  // a6
      "lw	x17, 56(sp);"  // a7
      "lw	x18, 52(sp);"  // s2
      "lw	x19, 48(sp);"  // s3
      "lw	x20, 44(sp);"  // s4
      "lw	x21, 40(sp);"  // s5
      "lw	x22, 36(sp);"  // s6
      "lw	x23, 32(sp);"  // s7
      "lw	x24, 28(sp);"  // s8
      "lw	x25, 24(sp);"  // s9
      "lw	x26, 20(sp);"  // s1
      "lw	x27, 16(sp);"  // s1
      "lw	x28, 12(sp);"  // t3
      "lw	x29,  8(sp);"  // t4
      "lw	x30,  4(sp);"  // t5
      "lw	x31,  0(sp);"  // t6
      "addi	sp,sp, 128;"   // Rewind SP to land at callee's SP
      // Cheat by copying 'ra' into mret return register before we return
      // This will effectively force the interrupt service that called us
      // to return back to the next instruction after LaunchELF()
      "csrrw zero, mepc, ra;"
      "ret;" : : "m" (savedstackpointer) :
   );
}

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // We only have illegal instruction handler installed,
   // therefore won't need to check the mcause register
   // to see why we're here

   uint32_t at = read_csr(mtval);         // PC
   uint32_t cause = read_csr(mcause)>>16; // Exception cause on bits [18:16]

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0) // ILLEGALINSTRUCTION
   {
      // Show the address and the failing instruction's opcode field
      UARTWrite("\n\nEXCEPTION: Illegal instruction 0x");
      UARTWriteHex(*(uint32_t*)at);
      UARTWrite(" at 0x");
      UARTWriteHex((uint32_t)at);
      UARTWrite("\n");
      // Stall
      while(1) { }
   }

   if ((cause&2) != 0) // EBREAK
   {
      // We've hit a breakpoint
      // TODO: Tie this into GDB routines (connected via UART)
      UARTWrite("\n\nEXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
      // Stall
      while(1) { }
   }

   if ((cause&4) != 0) // ECALL
   {
      uint32_t ecalltype = 0;
      asm volatile ("sw a7, %0 ;" : "=m" (ecalltype) : :);

      // register a7 contains the function code
      // a7: 0x5D terminate application
      // a7: 0x50 seems to be fread

      UARTWrite("\n\nECALL: 0x");
      UARTWriteHex(ecalltype);
      UARTWrite("\n");
      ReturnFromELF();
      // on return, mret return address now contains the 'ra' of the call to LaunchELF()
      // effectively tricking this interrupt handler to return to where we left off
   }
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

void LoadElfRunAddress()
{
   // Header data
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
}

void ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &jumptarget)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       UARTWrite(" failed: expecting 0x7F+'ELF'");
       return;
   }

   jumptarget = fheader->m_Entry;

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
      ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
      f_close(&fp);
      LaunchELF(branchaddress);
      return 0;
   }
   else // File not found but we'd prefer to be quiet about it
      return -1;
}

void ListDir(const char *path)
{
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
            if (strstr(finf.fname, ".elf"))
            {
               UARTWrite(finf.fname);
               UARTWrite("\n");
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      UARTWrite(FRtoString[re]);
}

void ProcessCommand(const char *cmd)
{
   if (strstr(cmd, "load") != nullptr)
   {
      const char *fname = cmd + 6;
      char fullpath[128];
      strcpy(fullpath, "sd:");
      strcat(fullpath, fname);
      LoadAndRunELF(fullpath);
   }

   if (strstr(cmd, "dir") != nullptr)
      ListDir("sd:");

   if (strstr(cmd, "help") != nullptr)
      UARTWrite("commands:\ndir:list SDCard root directory\nload prog.elf: load and run given program\n\n");
}

int main()
{
   InstallIllegalInstructionHandler();

   // Step A: BOOTLOADER phase - attempt to load boot.elf

   // Step A1: Try to mount the SDCard
   uint32_t sdcardavailable = 0;
   FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
   if (mountattempt!=FR_OK)
      UARTWrite(FRtoString[mountattempt]);
   else
      sdcardavailable = 1;

   int foundelf = 0;
   if (sdcardavailable)
   {
      // Step A2: Check to see if we can find a 'boot.elf' in root directory and run it
      foundelf = LoadAndRunELF("sd:boot.elf");
   }

   // Step B: UART phase - fallback to listening to UART when no boot.elf is found

   // Show startup info
   // This is done _after_ bootloader phase to give bootloader a chance to show different
   // or updated information, or none at all depending on system requirements.
   UARTWrite("\033[2J\n+-------------------------+\n|          ************** |\n| ########   ************ |\n| #########  ************ |\n| ########   ***********  |\n| #        ***********    |\n| ##   *************   ## |\n| ####   *********   #### |\n| ######   *****   ###### |\n| ########   *   ######## |\n| ##########   ########## |\n+-------------------------+\n");
   UARTWrite("\nNekoYon boot loader version N4:0003\nSingle core RISC-V(RV32IMFZicsr) @150Mhz\nDevices: UART,DDR3,SPI,GPU\n\u00A9 2021 Engin Cilasun\n\n");
   UARTWrite("No boot.elf available.\nListening to UART instead.\nUse command 'help' to list available functions.\n\n");

   char commandline[128];
   int commandlen = 0;
   if ((foundelf == -1) || (sdcardavailable == 0))
   {
      uint8_t prevchar = 0xFF;
      while(1)
      {
         // Step B1: Check to see if we have something in the FIFO
         if (*IO_UARTRXByteAvailable)
         {
            // Step B2: Read the data on UARTRX memory location
            char checkchar = *IO_UARTRXTX;
            *IO_UARTRXTX = checkchar; // Echo back

            // Step B3: Proceed to load binary blobs or run the incoming ELF
            // if we received a 'B\n' or 'R\n' sequence
            if (checkchar == 13)
            {
               UARTWrite("\n");
               // Rewind command
               commandlen = 0;

               // Load the incoming binary
               if (prevchar=='B')
                  LoadBinaryBlob();
               else if (prevchar=='R')
               {
                  LoadElfRunAddress();
                  LaunchELF(branchaddress);
               }
               else
                  ProcessCommand(commandline);
            }

            // Backspace?
            if (checkchar == 8)
            {
               commandline[commandlen--] = 0;
               if (commandlen <= 0) commandlen = 0;
            }
            else
            {
               commandline[commandlen++] = checkchar;
               commandline[commandlen] = 0;
               if (commandlen > 126) commandlen = 126;
            }

            // Step B4: Remember this character for the next round
            prevchar = checkchar;
         }
      }
   }

   return 0;
}
