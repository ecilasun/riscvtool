// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"
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

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) illegal_instruction_exception()
{
   // See https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf mcause section for the cause codes.

   uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
   uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16]
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
            if (incoming != 13)
            {
                commandline[cmdlen++] = incoming;
                commandline[cmdlen] = 0;
            }
            else
                parseit = 1;
            if (cmdlen>=511) cmdlen = 511;
            // Write back to UART
            *IO_UARTRXTX = incoming;
            UARTFlush();
         }

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
         UARTWrite("\033[0m\n\n"); // Clear attributes, step down a couple lines

         // reverse on: \033[7m
         // blink on: \033[5m
         // Set foreground color to red and bg color to black
         UARTWrite("\033[31m\033[40m");

         UARTWrite("┌───────────────────────────────────────────────────┐\n");
         UARTWrite("│ Software Failure. Press reset button to continue. │\n");
         UARTWrite("│        Guru Meditation #");
         UARTWriteHex((uint32_t)cause); // Cause
         UARTWrite(".");
         UARTWriteHex((uint32_t)value); // Failed instruction
         UARTWrite("         │\n");
         UARTWrite("└───────────────────────────────────────────────────┘\n");
         UARTWrite("\033[0m\n");

         // Put core to endless sleep
         while(1) {
            asm volatile("wfi;");
         }

         // Could alternatively handle each separately via:
         // switch (code)
         // {
         //    case CAUSE_ILLEGAL_INSTRUCTION:

         // Another use would be to do software emulation of the instruction in 'value'.
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

void DumpFile(const char *path)
{
    FIL fp;
    FRESULT fr = f_open(&fp, path, FA_READ);
    if (fr == FR_OK)
    {
        const uint32_t baseaddress = 0x00000000; // S-RAM
        UINT bytesread = 0;
        // Read at top of DDR3 memory
        f_read(&fp, (void*)baseaddress, 16384, &bytesread);
        f_close(&fp);
        // Dump contents
        for (uint32_t i=0;i<bytesread/4;++i)
        {
            const char chr = *((char*)(baseaddress+i));
            UARTPutChar(chr);
        }
    }
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
            //if (strstr(finf.fname, ".elf"))
            {
               UARTWrite(finf.fname);
               UARTWrite(" ");
               UARTWriteDecimal((int32_t)finf.fsize);
               UARTWrite("b\n");
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      UARTWrite(FRtoString[re]);
}

void domemtest()
{
    UARTWrite("\nTesting S-RAM on AXI4-Lite bus\n");

    UARTWrite("Data bus test (0x00000000-0x00003FFF)...");
    int failed = 0;
    for (uint32_t i=0x00000000; i<0x00003FFF; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    UARTWrite(failed == 0 ? "passed (" : "failed (");
    UARTWriteDecimal(failed);
    UARTWrite(" failures)\n");

    UARTWrite("Address bus test (0x00000000-0x00003FFF)...");
    int errortype = 0;
    datum* res = memTestAddressBus((volatile datum*)0x00000000, 16384, &errortype);
    UARTWrite(res == NULL ? "passed\n" : "failed\n");
    if (res != NULL)
    {
        if (errortype == 0)
            UARTWrite("Reason: Address bit stuck high at 0x");
        if (errortype == 1)
            UARTWrite("Reason: Address bit stuck low at 0x");
        if (errortype == 2)
            UARTWrite("Reason: Address bit shorted at 0x");
        UARTWriteHex((unsigned int)res);
        UARTWrite("\n");
    }

    UARTWrite("Memory device test (0x00000000-0x00003FFF)...");
    datum* res2 = memTestDevice((volatile datum *)0x00000000, 16384);
    UARTWrite(res2 == NULL ? "passed\n" : "failed\n");
    if (res2 != NULL)
    {
        UARTWrite("Reason: incorrect value read at 0x");
        UARTWriteHex((unsigned int)res2);
        UARTWrite("\n");
    }

    if ((failed != 0) | (res != NULL) | (res2 != NULL))
      UARTWrite("Scratchpad memory does not appear to be working correctly.\n");
}

void ParseCommands()
{
    if (!strcmp(commandline, "help")) // Help text
    {
        UARTWrite("dir, memtest, load filename, crash\n");
    }

    // See if we're requested to forcibly crash
    if (!strcmp(commandline, "crash")) // Test crash handler
    {
        UARTWrite("\nForcing crash via illegal instruction.\n");
        asm volatile(".dword 0xBADF00D0");
    }

    if (!strcmp(commandline, "dir")) // List directory
    {
		UARTWrite("\nListing files on volume sd:\n");
		ListFiles("sd:");
    }

    if (strstr(commandline, "load") != nullptr) // Load file to S-RAM
    {
        strcpy(filename, "sd:");
        strcat(filename, &(commandline[5]));

		UARTWrite("\nLoading '");
		UARTWrite(filename);
        UARTWrite("'\n");

		DumpFile(filename);
    }

    if (!strcmp(commandline, "memtest")) // Memory test on scratchpad
        domemtest();

    parseit = 0;
    cmdlen = 0;
    commandline[0]=0;
}

int main()
{
    InstallIllegalInstructionHandler();

    // Clear all attributes, clear screen, print boot message
    UARTWrite("\033[0m\033[2J\n");
    UARTWrite("┌──────┬─────────────────────────────────────────────────────┐\n");
    UARTWrite("│ CPU  │ E32 RISC-V (rv32imfZicsr) @100Mhz                   │\n");
    UARTWrite("│ BUS  │ AXI4-Lite @100MHz                                   │\n");
    UARTWrite("├──────┼─────────────────────────────────────────────────────┤\n");
    UARTWrite("│ SRAM │ 0x00000000-0x0001FFFF (Scratchpad, 128Kbytes)       │\n");
    UARTWrite("│ BRAM │ 0x10000000-0x1000FFFF (RAM/ROM v0006, 64Kbytes)     │\n");
    UARTWrite("│ UART │ 0x8000000X (X=8:R/W X=4:AVAIL X=0:FULL)             │\n");
    UARTWrite("│ SPI  │ 0x9000000X (X=0:R/W)                                │\n");
    UARTWrite("└──────┴─────────────────────────────────────────────────────┘\n\n");

    FATFS Fs;
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
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
