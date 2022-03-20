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
#include "buttons.h"
#include "ps2.h"
#include "ringbuffer.h"
#include "gpu.h"

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

static char currentdir[512] = "sd:";
static char commandline[512] = "";
static char filename[128] = "";
static int cmdlen = 0;
static int havedrive = 0;
static uint32_t numharts = 0;

// Main file system object
FATFS Fs;

// Keyboard event ring buffer (1024 bytes)
uint8_t *keyboardringbuffer = (uint8_t*)0x80000200; // 512 bytes into mailbox memory
// Keyboard map is at top of S-RAM (512 bytes)
uint16_t keymap[256];
// Previous key map to be able to track deltas (512 bytes)
uint16_t keymapprev[256];

void HandleUART()
{
    // TODO: This will eventually become the debug connection handler

    //*IO_UARTRXTX = 0x13; // XOFF
    //UARTFlush();

    // Echo back incoming bytes
    while ((*IO_UARTStatus)&0x00000001)
    {
        // Consume incoming character
        uint8_t incoming = *IO_UARTRX;
        // TODO: Handle debugger / file server etc
        // Echo back
        *IO_UARTTX = incoming;
    }
    UARTFlush();

    //*IO_UARTRXTX = 0x11; // XON
    //UARTFlush();
}

void HandleButtons()
{
    while (*BUTTONCHANGEAVAIL)
    {
        uint32_t change = *BUTTONCHANGE;
        UARTWrite("Button state change: ");
        UARTWriteHex(change);
        UARTWrite("\n");
    }
}

void HandleTimer()
{
    // Report number of HARTs found

    numharts = 1; // Count this HART first
    for (uint32_t i=1; i<32; ++i)
        numharts += (HARTMAILBOX[i] == 0xE32EBABE) ? 1 : 0;

    UARTWrite("\n\033[34m\033[47m\033[7m");
    UARTWrite("Number of HARTs detected: ");
    UARTWriteDecimal(numharts);
    UARTWrite("\033[0m\n");

    // Stop further timer interrupts by setting timecmp to furthest value available.
    swap_csr(0x801, 0xFFFFFFFF);
    swap_csr(0x800, 0xFFFFFFFF);
}

void HandleKeyboard()
{
    // Consume all key state changes from FIFO and update the key map
    while (*PS2KEYBOARDDATAAVAIL)
        ScanKeyboard(keymap);

    // If there's a difference between the previous keymap and current one, generate events for each change
    for (uint32_t i=0; i<256; ++i)
    {
        // Skip keyboard OK (when plugged in)
        if (i==0xAA)
            continue;

        // Generate key up/down events
        uint32_t prevval = (uint32_t)keymapprev[i];
        uint32_t val = (uint32_t)keymap[i];
        if (prevval^val) // Mismatch, this considered an event
        {
            // Store new state in previous state buffer since we're done reading it
            keymapprev[i] = val;
            // Wait for adequate space in ring buffer to write
            // NOTE: We'll simply hang if ringbuffer is full because nothing else
            // is running on this core during ISR. So attempt once, and bail out
            // if we can't write...
            //while(RingBufferWrite(keyboardringbuffer, &val, 4) == 0) { }
            RingBufferWrite(keyboardringbuffer, &val, 4);
        }
    }
}

void HandleTrap(const uint32_t cause, const uint32_t a7, const uint32_t value, const uint32_t PC)
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

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) interrupt_service_routine()
{
    uint32_t a7;
    asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined.

    uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
    uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
    uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
    uint32_t code = cause & 0x7FFFFFFF;

    if (cause & 0x80000000) // Interrupt
    {
        if (code == 0xB) // Machine External Interrupt (hardware)
        {
            // Route based on hardware type
            if (value & 0x00000001) HandleUART();
            if (value & 0x00000002) HandleButtons();
            if (value & 0x00000004) HandleKeyboard();
        }

        if (code == 0x7) // Machine Timer Interrupt (timer)
        {
            HandleTimer();
        }
    }
    else // Machine Software Exception (trap)
    {
        HandleTrap(cause, a7, value, PC);
    }
}

void InstallISR()
{
   // Set machine trap vector
   swap_csr(mtvec, interrupt_service_routine);

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

void __attribute__((aligned(256))) __attribute__((interrupt("machine"))) worker_interrupt_service_routine()
{
    uint32_t a7;
    asm volatile ("sw a7, %0;" : "=m" (a7)); // Catch value of A7 before it's ruined.

    //uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
    uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
    //uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
    uint32_t code = cause & 0x7FFFFFFF;

    if (cause & 0x80000000) // Interrupt
    {
        if (code == 0xB) // Machine External Interrupt (hardware)
        {
            // Route based on hardware type
            /*if (value & 0x00000001) HandleUART();
            if (value & 0x00000002) HandleButtons();
            if (value & 0x00000004) HandleKeyboard();*/
        }

        if (code == 0x7) // Machine Timer Interrupt (timer)
        {
            uint32_t hartid = read_csr(mhartid);
            HARTMAILBOX[hartid] = 0xE32EBABE; // Awake
            // Stop further timer interrupts by setting timecmp to furthest value available.
            swap_csr(0x801, 0xFFFFFFFF);
            swap_csr(0x800, 0xFFFFFFFF);
            //HandleTimer();
        }
    }
    else // Machine Software Exception (trap)
    {
        //HandleTrap(cause, a7, value, PC);
    }
}

void InstallWorkerISR()
{
   // Set machine trap vector
   swap_csr(mtvec, worker_interrupt_service_routine);

   // Set up timer interrupt a few units into the future
   uint64_t now = E32ReadTime();
   uint64_t future = now + 512; // Set to happen very soon, around similar points in time for all HARTs except #0
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

void CLF()
{
    // Write to FB0, display FB1
    *GPUCTL = 0;

    // Clear frame buffer
    for (int y=0;y<240;++y)
      for (int x=0;x<80;++x)
        GPUFBWORD[x+y*80] = 0x020C200C;

    // Write to FB1, display FB0
    *GPUCTL = 1;
}

void CLS()
{
    UARTWrite("\033[0m\033[2J");
}

void FlushDataCache()
{
    // Force D$ flush so that contents are visible by I$
    // We do this by forcing a dummy load 4096 words from
    // a single cache page, to force previous contents to be
    // written back to DDR3
    // (P.S. 4096 words == 16384 bytes == size of D$)
    volatile uint32_t *DDR3Start = (volatile uint32_t *)0x1F000000;

    // NOTE: This needs to be replaced with FENCE.I so that
    // the I$ tags are invalidated instead, so it's forced
    // to re-load, versus trying to get D$ to flush.
    for (uint32_t i=0; i<4096; ++i)
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
    // Grab first token, if any
    const char *command = strtok(commandline, " ");

    if (!strcmp(command, "help")) // Help text
    {
        UARTWrite("\033[34m\033[47m\033[7mdir\033[0m: show list of files in working directory\n");
        UARTWrite("\033[34m\033[47m\033[7mcwd\033[0m: change working directory\n");
        UARTWrite("\033[34m\033[47m\033[7mpwd\033[0m: show working directory\n");
        UARTWrite("\033[34m\033[47m\033[7mcls\033[0m: clear visible portion of terminal\n");
    }
    else if (!strcmp(command, "cwd"))
    {
        // Use first parameter to set current directory
        char *param = strtok(nullptr, " ");
        if (param != nullptr)
            strcpy(currentdir, param);
        else
            strcpy(currentdir, "sd:");
    }
    else if (!strcmp(command, "pwd"))
    {
        UARTWrite(currentdir);
        UARTWrite("\n");
    }
    else if (!strcmp(command, "dir")) // List directory
    {
		UARTWrite("\n");
        UARTWrite(currentdir);
        UARTWrite("\n");
		ListFiles(currentdir);
    }
    else if (!strcmp(command, "cls")) // Clear terminal screen
    {
        CLS();
    }
    else if (command!=nullptr) // None, assume this is a program name at the working directory of the SDCard
    {
        // Build a file name from the input string
        strcpy(filename, currentdir);
        strcat(filename, command);
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

            // Unmount filesystem and reset to root directory before passing control
            f_mount(nullptr, "sd:", 1);
            strcpy(currentdir, "sd:");
            havedrive = 0;

            FlushDataCache(); // Make sure we've forced a cache flush on the D$ (TODO: Use FENCE.I here instead, once it's implemented)

            asm volatile(
                "lw s0, %0;" // Target loaded in S-RAM top (uncached, doesn't need D$->I$ flush)
                "jalr s0;" // Branch with the intent to return back here
                : "=m" (branchaddress) : : 
            );

            // Re-mount filesystem before re-gaining control
            f_mount(&Fs, "sd:", 1);
            havedrive = 1;
        }
        else
        {
            UARTWrite("Executable '");
            UARTWrite(filename);
            UARTWrite("' not found.\n");
        }
    }

    cmdlen = 0;
    commandline[0]=0;
}

int ProcessKeyEvents()
{
    static int uppercase = 0;
    int parseit = 0;

    // Any pending keyboard events to handle?
    uint32_t val = 0;
    // Consume one entry per execution
    swap_csr(mie, MIP_MSIP | MIP_MTIP);
    int R = RingBufferRead(keyboardringbuffer, &val, 4);
    swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);
    if (R)
    {
        uint32_t key = val&0xFF;
        // Toggle caps
        if (key == 0x58)
            uppercase = !uppercase;

        // Key up
        if (val&0x00000100)
        {
            if (key == 0x12 || key == 0x59) // Right/left shift up
                uppercase = 0;
        }
        else // Key down
        {
            if (key == 0x12 || key == 0x59) // Right/left shift down
                uppercase = 1;
            else
            {
                if (key == 0x5A) // Enter
                    parseit = 1;
                else if (key == 0x66) // Backspace
                {
                    cmdlen--;
                    if (cmdlen < 0) cmdlen = 0;
                    commandline[cmdlen] = 0;
                }
                else
                {
                    commandline[cmdlen++] = ScanToASCII(val, uppercase);
                    if (cmdlen>=511)
                        cmdlen = 511;
                    commandline[cmdlen] = 0;
                }
                UARTPutChar(ScanToASCII(val, uppercase));
            }
        }
    }

    return parseit;
}

// Worker CPU entry point
void workermain()
{
    InstallWorkerISR();
    while (1)
    {
        // Halt on wakeup
        asm volatile("wfi;");
        // NOTE: Our ISR will service this interrupt
    }
}

int main()
{
    // Interrupt service routine
    InstallISR();

    // Clear frame buffer
    CLF();

    // Clear all attributes, clear screen, print boot message
    CLS();
    UARTWrite("┌────────────────────────────────────┐\n");
    UARTWrite("│ E32OS v0.150 (c)2022 Engin Cilasun │\n");
    UARTWrite("└────────────────────────────────────┘\n\n");

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

        // Process any keyboard events produced by the ISR and
        // parse the command generated in the command line on Enter.
        if (ProcessKeyEvents())
            ParseCommands();
    }

    return 0;
}
