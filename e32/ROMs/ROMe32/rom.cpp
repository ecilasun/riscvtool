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

/*******************************/

float __attribute((naked)) test_div()
{
    asm volatile (
        "la a0, test_div_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fdiv.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_div_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_mul()
{
    asm volatile (
        "la a0, test_mul_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmul.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_mul_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_add()
{
    asm volatile (
        "la a0, test_add_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fadd.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_add_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_sub()
{
    asm volatile (
        "la a0, test_sub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsub.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_sub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_min()
{
    asm volatile (
        "la a0, test_min_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmin.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_min_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_max()
{
    asm volatile (
        "la a0, test_max_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmax.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_max_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_sqrt()
{
    asm volatile (
        "la a0, test_sqrt_data ;"
        "flw fs1, 4(a0) ;"
        "fsqrt.s fa0, fs1 ;"
        "ret ;"
        "test_sqrt_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_madd()
{
    asm volatile (
        "la a0, test_madd_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fmadd.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_madd_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_msub()
{
    asm volatile (
        "la a0, test_msub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fmsub.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_msub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_nmsub()
{
    asm volatile (
        "la a0, test_nmsub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fnmsub.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_nmsub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_nmadd()
{
    asm volatile (
        "la a0, test_nmadd_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fnmadd.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_nmadd_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

int __attribute((naked)) test_fcvtws()
{
    asm volatile (
        "la a0, test_cvtws_data ;"
        "flw fs0, 0(a0) ;"
        "fcvt.w.s a0, fs0, rtz ;"
        "ret ;"
        "test_cvtws_data: ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fcvtsw()
{
    asm volatile (
        "la a0, test_cvtsw_data ;"
        "lw a1, 0(a0) ;"
        "fcvt.s.w fa0, a1 ;"
        "ret ;"
        "test_cvtsw_data: ;"
        ".int 554 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnj()
{
    asm volatile (
        "la a0, test_sgnj_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnj.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnj_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnjn()
{
    asm volatile (
        "la a0, test_sgnjn_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnjn.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnjn_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnjx()
{
    asm volatile (
        "la a0, test_sgnjx_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnjx.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnjx_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_flt()
{
    asm volatile (
        "la a0, test_lt_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flt.s a0,fs0,fs1 ;"
        "ret ;"
        "test_lt_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_fle()
{
    asm volatile (
        "la a0, test_le_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fle.s a0,fs0,fs1 ;"
        "ret ;"
        "test_le_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_feq()
{
    asm volatile (
        "la a0, test_eq_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "feq.s a0,fs0,fs1 ;"
        "ret ;"
        "test_eq_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_fmvxw()
{
    asm volatile (
        "la a0, test_mvxw_data ;"
        "flw fs0, 0(a0) ;"
        "fmv.x.w a0,fs0 ;"
        "ret ;"
        "test_mvxw_data: ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fmvwx()
{
    asm volatile (
        "la a0, test_mvwx_data ;"
        "lw a1, 0(a0) ;"
        "fmv.w.x fa0,a1 ;"
        "ret ;"
        "test_mvwx_data: ;"
        ".int 0x440a6271 ;" // 553.53814697265625
        :  : :
    );
}

void FPUTest()
{
    setbuf(stdout, NULL);

    const float a = 1.3124145f;
    const float b = 553.538131f;
    const float c = 33.784341f;
    const float d = -111.5555f;
    float r = 0.f;
    int t = 554;
    int s = 0x440a6271;
    int ri = 0;

    UARTWrite("Testing FPU instructions\n\n");

    UARTWrite("fdiv\n");
    r = test_div();
    //printf("fdiv: %f / %f = %f\n", a, b, r);

    UARTWrite("fmul\n");
    r = test_mul();
    //printf("fmul: %f * %f = %f\n", a, b, r);

    UARTWrite("fadd\n");
    r = test_add();
    //printf("fadd: %f + %f = %f\n", a, b, r);

    UARTWrite("fsub\n");
    r = test_sub();
    //printf("fsub: %f - %f = %f\n", a, b, r);

    UARTWrite("fmin\n");
    r = test_min();
    //printf("fmin: min(%f,%f) = %f\n", a, b, r);

    UARTWrite("fmax\n");
    r = test_max();
    //printf("fmax: max(%f,%f) = %f\n", a, b, r);

    UARTWrite("fsqrt\n");
    r = test_sqrt();
    //printf("fsqrt: sqrt(%f) = %f\n", b, r);

    UARTWrite("fmadd\n");
    r = test_madd();
    //printf("fmadd: %f*%f+%f = %f\n", a, b, c, r);

    UARTWrite("fmsub\n");
    r = test_msub();
    //printf("fmsub: %f*%f-%f = %f\n", a, b, c, r);

    UARTWrite("fnmsub\n");
    r = test_nmsub();
    //printf("fnmsub: -%f*%f+%f = %f\n", a, b, c, r);

    UARTWrite("fnmadd\n");
    r = test_nmadd();
    //printf("fnmadd: -%f*%f-%f = %f\n", a, b, c, r);

    UARTWrite("fcvt.w.s\n");
    ri = test_fcvtws();
    //printf("fcvt.w.s: %f = %d\n", b, ri);

    UARTWrite("fcvt.s.w\n");
    r = test_fcvtsw();
    //printf("fcvt.s.w: %d = %f\n", t, r);

    UARTWrite("fsgnj.s\n");
    r = test_fsgnj();
    //printf("fsgnj.s: sgnj(%f,%f) = %f\n", d, b, r);

    UARTWrite("fsgnjn.s\n");
    r = test_fsgnjn();
    //printf("fsgnjn.s: sgnj(%f,%f) = %f\n", d, b, r);

    UARTWrite("fsgnjx.s\n");
    r = test_fsgnjx();
    //printf("fsgnjx.s: sgnj(%f,%f) = %f\n", d, b, r);

    UARTWrite("flt.s\n");
    ri = test_flt();
    //printf("flt.s: %f < %f ? = %d\n", d, b, ri);

    UARTWrite("fle.s\n");
    ri = test_fle();
    //printf("fle.s: %f <= %f ? = %d\n", d, b, ri);

    UARTWrite("feq.s\n");
    ri = test_feq();
    //printf("feq.s: %f == %f ? = %d\n", d, b, ri);

    UARTWrite("fmv.x.w\n");
    ri = test_fmvxw();
    //printf("fmv.x.w: %f -> %.8X\n", b, ri);

    UARTWrite("fmv.w.x\n");
    r = test_fmvwx();
    //printf("fmv.w.x: %.8X -> %f\n", s, r);

    UARTWrite("FPU instruction test complete\n");
}

/*******************************/

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
    int failed = 0;
    for (uint32_t i=0x01000000; i<0x01000F00; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    UARTWrite("Walking-1s test (0x01000000-0x01000F00)\n");
    UARTWriteDecimal(failed);
    UARTWrite(" failures\n");

    datum* res = memTestAddressBus((volatile datum*)0x01000000, 4096);
    UARTWrite("Address bus test (0x01000000-4Kbytes)\n");
    UARTWrite(res == NULL ? "passed" : "failed");
    UARTWrite("\n");
    if (res != NULL)
    {
        UARTWrite("Reason: address aliasing problem at 0x");
        UARTWriteHex((unsigned int)res);
        UARTWrite("\n");
    }

    datum* res2 = memTestDevice((volatile datum *)0x01000000, 4096);
    UARTWrite("Memory device test (0x01000000-4Kbytes)\n");
    UARTWrite(res2 == NULL ? "passed" : "failed");
    UARTWrite("\n");
    if (res2 != NULL)
    {
        UARTWrite("Reason: incorrect value read at 0x");
        UARTWriteHex((unsigned int)res2);
        UARTWrite("\n");
    }

    if ((failed != 0) | (res != NULL) | (res2 != NULL))
      UARTWrite("DDR3 device does not appear to be working correctly, or does not exist.\n");
}

void ParseCommands()
{
    if (!strcmp(commandline, "help")) // Help text
    {
        UARTWrite("crash, dir, ddr3, fpu\n");
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

    if (!strcmp(commandline, "ddr3")) // Test the DDR3
        domemtest();

    if (!strcmp(commandline, "fpu")) // Test the FPU instructions
        FPUTest();

    parseit = 0;
    cmdlen = 0;
    commandline[0]=0;
}

int main()
{
    InstallIllegalInstructionHandler();

    // Clear all attributes, clear screen, print boot message
    UARTWrite("\033[0m\033[2J\n");
    UARTWrite("┌──────┬─────────────────────────────────────────┐\n");
    UARTWrite("│ CPU  │ E32 RISC-V (RV32iMFZicsr) @100Mhz       │\n");
    UARTWrite("├──────┼─────────────────────────────────────────┤\n");
    UARTWrite("│ ROM  │ 0x00000000-0x0000FFFF v0006             │\n");
    UARTWrite("│ RAM  │ 0x00000000-0x0000FFFF                   │\n");
    UARTWrite("│ UART │ 0x8000000X (X=8:R/W X=4:AVAIL X=0:FULL) │\n");
    UARTWrite("│ SPI  │ 0x9000000X (X=0:R/W)                    │\n");
    UARTWrite("└──────┴─────────────────────────────────────────┘\n\n");

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
