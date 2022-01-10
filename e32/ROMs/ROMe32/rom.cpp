// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"
#include "elf.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "memtest/memtest.h"

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

static char commandline[512]="";
static char filename[128]="";
static int cmdlen = 0;
static int parseit = 0;
static int havedrive = 0;

// Shared FAT file system at bottom of S-RAM
FATFS *Fs = (FATFS*)0x8002F000;

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
    // Trap A7 before it's ruined.
    uint32_t a7;
    asm volatile ("sw a7, %0;" : "=m" (a7));

    // See https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf mcause section for the cause codes.

    uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
    uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16]
    uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
    uint32_t code = cause & 0x7FFFFFFF;

    if (cause & 0x80000000) // Interrupt
    {
        if (code == 0xB) // hardware
        {
            //*IO_UARTRXTX = 0x13; // XOFF
            //UARTFlush();

            // Echo back incoming bytes
            while (*IO_UARTRXByteAvailable)
            {
                // Read incoming character
                uint8_t incoming = *IO_UARTRXTX;
                // Zero terminated command line
                if (incoming == 13)
                    parseit = 1;
                else if (incoming == 8)
                {
                    cmdlen--;
                    if (cmdlen < 0) cmdlen = 0;
                    commandline[cmdlen] = 0;
                }
                else
                {
                    commandline[cmdlen++] = incoming;
                    if (cmdlen>=511) cmdlen = 511;
                    commandline[cmdlen] = 0;
                }
                // Write back to UART
                *IO_UARTRXTX = incoming;
            }
            UARTFlush();

            //*IO_UARTRXTX = 0x11; // XON
            //UARTFlush();
        }
        if (code == 0x7) // timer
        {
            UARTWrite("\n\033[34m\033[47m\033[7m| ");
            UARTWrite("HINT: Type 'help' for a list of commands.");
            UARTWrite(" │\033[0m\n");
            // Stop further timer interrupts by setting the timecmp to furthest value available.
            swap_csr(0x801, 0xFFFFFFFF);
            swap_csr(0x800, 0xFFFFFFFF);
        }
    }
    else // Exception
    {
        // NOTE: One use of illegal instruction exception would be to do software emulation of the instruction in 'value'.
        switch (cause)
        {
            case CAUSE_BREAKPOINT:
                // TODO: Debugger related
            break;

            /*case CAUSE_USER_ECALL:
            case CAUSE_SUPERVISOR_ECALL:
            case CAUSE_HYPERVISOR_ECALL:*/
            case CAUSE_MACHINE_ECALL:

                // NOTE: See \usr\include\asm-generic\unistd.h for a full list

                // A7
                // 64  sys_write  -> print (A0==1->stdout, A1->string, A2->length)
                // 96  sys_exit   -> terminate (A0==return code)
                // 116 sys_syslog -> 
                // 117 sys_ptrace -> 

                // TODO: implement system calls

                UARTWrite("\033[31m\033[40m");

                UARTWrite("┌───────────────────────────────────────────────────┐\n");
                UARTWrite("│ Unimplemented machine ECALL. Program will resume  │\n");
                UARTWrite("│ execution, though it might crash.                 │\n");
                UARTWrite("│ #");
                UARTWriteHex((uint32_t)a7); // A7 containts Syscall ID
                UARTWrite(" @");
                UARTWriteHex((uint32_t)PC); // PC
                UARTWrite("                               │\n");
                UARTWrite("└───────────────────────────────────────────────────┘\n");
                UARTWrite("\033[0m\n");
            break;

            /*case CAUSE_MISALIGNED_FETCH:
            case CAUSE_FETCH_ACCESS:
            case CAUSE_ILLEGAL_INSTRUCTION:
            case CAUSE_MISALIGNED_LOAD:
            case CAUSE_LOAD_ACCESS:
            case CAUSE_MISALIGNED_STORE:
            case CAUSE_STORE_ACCESS:
            case CAUSE_FETCH_PAGE_FAULT:
            case CAUSE_LOAD_PAGE_FAULT:
            case CAUSE_STORE_PAGE_FAULT:*/
            default:
            {
                UARTWrite("\033[0m\n\n"); // Clear attributes, step down a couple lines

                // reverse on: \033[7m
                // blink on: \033[5m
                // Set foreground color to red and bg color to black
                UARTWrite("\033[31m\033[40m");

                UARTWrite("┌───────────────────────────────────────────────────┐\n");
                UARTWrite("│ Software Failure. Press reset button to continue. │\n");
                UARTWrite("│   Guru Meditation #");
                UARTWriteHex((uint32_t)cause); // Cause
                UARTWrite(".");
                UARTWriteHex((uint32_t)value); // Failed instruction
                UARTWrite(" @");
                UARTWriteHex((uint32_t)PC); // PC
                UARTWrite("    │\n");
                UARTWrite("└───────────────────────────────────────────────────┘\n");
                UARTWrite("\033[0m\n");

                // Put core to endless sleep
                while(1) {
                    asm volatile("wfi;");
                }
                break; // Doesn't make sense but to make compiler happy...
            }
        }
    }
}

void InstallIllegalInstructionHandler()
{
   // Set machine trap vector
   swap_csr(mtvec, illegal_instruction_exception);

   // Set up timer interrupt one second into the future
   uint64_t now = E32ReadTime();
   uint64_t future = now + ONE_SECOND_IN_TICKS; // One second into the future (based on 10MHz wall clock)
   E32SetTimeCompare(future);

   // Enable machine software interrupts (breakpoint/illegal instruction)
   // Enable machine hardware interrupts
   // Enable machine timer interrupts
   swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

   // Allow all machine interrupts to trigger
   swap_csr(mstatus, MSTATUS_MIE);
}

void ListFiles(const char *path)
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
            char *isexe = strstr(finf.fname, ".elf");
            int isdir = finf.fattrib&AM_DIR;
            if (isdir)
                UARTWrite("\033[32m"); // Green
            if (isexe!=nullptr)
                UARTWrite("\033[33m"); // Yellow
            UARTWrite(finf.fname);
            if (isdir)
                UARTWrite(" <dir>");
            else
            {
                UARTWrite(" ");
                UARTWriteDecimal((int32_t)finf.fsize);
                UARTWrite("b");
            }
            UARTWrite("\033[0m\n");
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      UARTWrite(FRtoString[re]);
}

void CLS()
{
    UARTWrite("\033[0m\033[2J");
}

void FlushDataCache()
{
   // Force D$ flush so that contents are visible by I$
   // We do this by forcing a dummy load of DWORDs from 0 to 2048
   // to force previous contents to be written back to DDR3
   // Tag is on high 17 bits, use end-of-memory to make sure we
   // access somewhere not touched by program loads
   volatile uint32_t *DDR3Start = (volatile uint32_t *)0x00000000;
   for (uint32_t i=0; i<2048; ++i)
   {
      uint32_t dummyread = DDR3Start[i];
      // This is to make sure compiler doesn't eat our reads
      // and shuts up about unused variable
      asm volatile ("add x0, x0, %0;" : : "r" (dummyread) : );
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
         /*UARTWriteHex(sheader.m_Addr);
         UARTWrite(" @");
         UARTWriteHex(sheader.m_Offset);
         UARTWrite(" ");
         UARTWriteHex(sheader.m_Size);
         UARTWrite("\n");*/
         // ...place it in memory
         uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr;
         f_lseek(fp, sheader.m_Offset);
         f_read(fp, elfsectionpointer, sheader.m_Size, &bytesread);
      }
   }

   free(names);
}

void ParseCommands()
{
    if (!strcmp(commandline, "help")) // Help text
    {
        UARTWrite("dir, cls\n");
    }
    else if (!strcmp(commandline, "dir")) // List directory
    {
		UARTWrite("\nListing files on volume sd:\n");
		ListFiles("sd:");
    }
    else if (!strcmp(commandline, "cls")) // Clear terminal screen
    {
        CLS();
    }
    else // Unknown command, assume this is a program name from root directory of the SDCard
    {
        if (strlen(commandline)>1)
        {
            // Build a file name from the input string
            strcpy(filename, "sd:");
            strcat(filename, commandline);
            strcat(filename, ".elf");

            FIL fp;
            FRESULT fr = f_open(&fp, filename, FA_READ);
            if (fr == FR_OK)
            {
                SElfFileHeader32 fheader;
                UINT readsize;
                f_read(&fp, &fheader, sizeof(fheader), &readsize);
                uint32_t branchaddress;
                ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
                f_close(&fp);

                // Unmount filesystem before passing control
                f_mount(nullptr, "sd:", 1);

                FlushDataCache(); // Make sure we've forced a cache flush on the D$
                UARTWrite("Starting ");
                UARTWrite(filename);
                UARTWrite("...\n");

                asm volatile(
                    "lw s0, %0;" // Target loaded in S-RAM top (uncached, doesn't need D$->I$ flush)
                    "jalr s0;" // Branch with the intent to return back here
                    : "=m" (branchaddress) : : 
                );

                // Re-mount filesystem before passing control
                f_mount(Fs, "sd:", 1);
            }
            else
            {
                UARTWrite("Executable ");
                UARTWrite(filename);
                UARTWrite(" not found\n");
            }
        }
    }

    parseit = 0;
    cmdlen = 0;
    commandline[0]=0;
}

int main()
{
    InstallIllegalInstructionHandler();

    // Clear all attributes, clear screen, print boot message
    CLS();
    UARTWrite("┌───────────────────────────────────┐\n");
    UARTWrite("│ E32OS v0.12 (c)2022 Engin Cilasun │\n");
    UARTWrite("└───────────────────────────────────┘\n\n");

	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
    {
        havedrive = 0;
		UARTWrite(FRtoString[mountattempt]);
    }
	else
        havedrive = 1;

    while(1)
    {
        // Interrupt handler will do all the real work.
        // Therefore we can put the core to sleep until an interrupt occurs,
        // after which it will wake up to service it and then go back to
        // sleep, unless we asked it to crash.
        asm volatile("wfi;");

        if (parseit)
            ParseCommands();
    }

    return 0;
}
