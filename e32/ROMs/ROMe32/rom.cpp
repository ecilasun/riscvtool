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

static volatile int donotcrash = 0xDADED0D1;

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
            // Force crash to test re-entering exception handler
            donotcrash = (incoming == 'C') ? 0x00000000 : 0xDADED0D1;
            // Write back to UART
            *IO_UARTRXTX = incoming;
            UARTFlush();
         }

         //*IO_UARTRXTX = 0x11; // XON
         //UARTFlush();
      }
      if (code == 0x7) // timer
      {
         UARTWrite("\n\033[34m\033[47m\033[7mTEST: One-shot timer trap\033[0m\n");
         // Stop further timer interrupts by setting the timecmp to furthest value available.
         swap_csr(0x801, 0xFFFFFFFF);
         swap_csr(0x800, 0xFFFFFFFF);
      }
   }
   else // Exception
   {
      switch (code)
      {
         case CAUSE_ILLEGAL_INSTRUCTION:
         {
            // Trap and stop execution.
            UARTWrite("\n\n\033[31m\033[47m\033[7mEXCEPTION: Illegal instruction word 0x");
            UARTWriteHex((uint32_t)value);
            UARTWrite("\033[0m\n");
            // Put core to sleep
            while(1) {
               asm volatile("wfi;");
            }
         }

         default:
         {
            // Trap and resume execution, for the time being.
            UARTWrite("\n\n\033[31m\033[47m\033[7mEXCEPTION:  mcause = 0x");
            UARTWriteHex((uint32_t)cause);
            UARTWrite(" mtval = 0x");
            UARTWriteHex((uint32_t)value);
            UARTWrite("\033[0m\n");
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

void ListELF(const char *path)
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
			if (strstr(finf.fname, ".elf"))
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

int main()
{
   InstallIllegalInstructionHandler();

   // Clear all attributes, clear screen, print boot message
   UARTWrite("\033[0m\033[2J\nE32: RV32iZicsr\nROM v0003\n");

	FATFS Fs;
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		UARTWrite(FRtoString[mountattempt]);
	else
	{
		UARTWrite("Listing ELF files on sd:\n");
		// List ELF files on the mounted volume
		ListELF("sd:");
	}

   while(1)
   {
      // Interrupt handler will do all the real work.
      // Therefore we can put the core to sleep until an interrupt occurs,
      // after which it will wake up to service it and then go back to
      // sleep, unless we asked it to crash.
      asm volatile("wfi;");

      // See if we're requested to forcibly crash
      if (donotcrash == 0x00000000)
      {
         UARTWrite("\nForcing illegal instruction exception...\n");
         asm volatile(".dword 0x012345FF");
         asm volatile(".dword 0xFFFFFFFF");
      }
   }

   return 0;
}
