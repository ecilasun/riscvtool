// NekoIchi ROM
// Experimental

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
#include "debugger.h"

// NOTE: Uncomment when experimental ROM should be compiled as actual ROM image
// Also need to swap the the ROM image compile command in build.sh file
//#define STARTUP_ROM
//#include "rom_nekoichi_rvcrt0.h"

// 10ms default task switch interval
#define DEFAULT_TIMESLICE 100000

// Currently supporting this many tasks
#define MAX_TASKS 16

// Entry for a task's state
struct cpu_context
{
   uint32_t reg[64]{0}; // Task's saved registers (not storing float registers yet)
   uint32_t SP{0}; // Task's saved stack pointer
   uint32_t PC{0}; // Task's saved program counter
   uint32_t quantum{DEFAULT_TIMESLICE}; // Run time for this task
};

// Entry zero will always be main()
uint32_t current_task = 0; // init_task
cpu_context task_array[MAX_TASKS];
uint32_t num_tasks = 0; // only one initially, which is the init_task
uint32_t have_a_break = 0;

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

// Demo book keeping storage
uint32_t cursorevenodd = 0;
uint32_t showtime = 0;
uint32_t escapemode = 0;
uint64_t clk = 0;
uint32_t milliseconds = 0;
uint32_t hours, minutes, seconds;
uint32_t breakpoint_triggered = 0;
uint32_t saved_instruction = 0;

void RunELF()
{
    // Set up stack pointer and branch to loaded executable's entry point (noreturn)
    // TODO: Can we work out the stack pointer to match the loaded ELF's layout?
    asm volatile (
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
        // NOTE: Since we have many stacks in this executable, choose the deepest in memory for new exec.
        "li x12, 0x0003FFF0 \n"
        "mv sp, x12 \n"
        "ret \n"
        : 
        : "m" (branchaddress)
        : 
    );
}

void ParseELFHeader(SElfFileHeader32 *fheader)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       EchoConsole("Failed: expecting 0x7F+'ELF'\n");
       return;
   }

   branchaddress = fheader->m_Entry;
   EchoConsole("Program entry point: "); EchoUART("Program entry point: ");
   EchoConsoleHex(branchaddress); EchoInt(branchaddress);
   EchoConsole("\r\n"); EchoUART("\r\n");

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
      }
   }

   free(names);
}

int LoadELF(const char *filename)
{
   FRESULT fr = pf_open(filename);
   if (fr == FR_OK)
   {
      // File size: Fs.fsize
      WORD bytesread;
      // Read header
      SElfFileHeader32 fheader;
      pf_read(&fheader, sizeof(fheader), &bytesread);
      ParseELFHeader(&fheader);
      return 0;
   }
   else
   {
      EchoConsole("Not found:"); EchoUART("Not found:");
      EchoConsole(filename); EchoUART(filename);
      EchoConsole("\r\n"); EchoUART("\r\n");
      return -1;
   }
}

void ListDir()
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
            EchoConsole(finf.fname); EchoUART(finf.fname);
            EchoConsole(" "); EchoUART(" ");
            EchoConsole(finf.fsize); EchoInt(finf.fsize);
            EchoConsole(" "); EchoUART(" ");
            /*EchoConsole(1944 + ((finf.ftime&0xFE00)>>9));
            EchoConsole("/");
            EchoConsole((finf.ftime&0x1E0)>>5);
            EchoConsole("/");
            EchoConsole(finf.ftime&0x1F);*/
            if (finf.fattrib&0x01) { EchoConsole("r"); EchoUART("r"); }
            if (finf.fattrib&0x02) { EchoConsole("h"); EchoUART("h"); }
            if (finf.fattrib&0x04) { EchoConsole("s"); EchoUART("s"); }
            if (finf.fattrib&0x08) { EchoConsole("l"); EchoUART("l"); }
            if (finf.fattrib&0x0F) { EchoConsole("L"); EchoUART("L"); }
            if (finf.fattrib&0x10) { EchoConsole("d"); EchoUART("d"); }
            if (finf.fattrib&0x20) { EchoConsole("a"); EchoUART("a"); }
            EchoConsole("\r\n"); EchoUART("\r\n");
         }
      } while(re == FR_OK && dir.sect!=0);
   }
   else
      EchoUART(FRtoString[re]);
}

void HandleDemoCommands(char checkchar)
{
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
      char cmdbuffer[36];

      // First, copy current row
      ConsoleStringAtRow(cmdbuffer);
      // Next, send a newline to go down one
      EchoConsole("\r\n");
      EchoUART("\r\n");

      // Clear the whole screen
      if ((cmdbuffer[0]=='c') && (cmdbuffer[1]=='l') && (cmdbuffer[2]=='s'))
      {
         ClearConsole();
      }
      else if ((cmdbuffer[0]=='h') && (cmdbuffer[1]=='e') && (cmdbuffer[2]=='l') && (cmdbuffer[3]=='p'))
      {
         EchoConsole("dir: list files\r\n");
         EchoConsole("load filename: load and run ELF\n\r");
         EchoConsole("cls: clear screen\r\n");
         EchoConsole("help: help screen\r\n");
         EchoConsole("time: toggle time\r\n");
         EchoConsole("run: branch to entrypoint\r\n");
      }
      else if ((cmdbuffer[0]=='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='r'))
      {
         ListDir();
      }
      else if ((cmdbuffer[0]=='l') && (cmdbuffer[1]=='o') && (cmdbuffer[2]=='a') && (cmdbuffer[3]=='d'))
      {
         LoadELF(&cmdbuffer[5]); // Skip 'load ' part
      }
      else if ((cmdbuffer[0]=='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
         showtime = (showtime+1)%2;
      else if ((cmdbuffer[0]=='r') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='n'))
         RunELF();
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

void ProcessUARTInputAsync()
{
   while (*IO_UARTRXByteCount)
   {
      char checkchar = *IO_UARTRX;
      HandleDemoCommands(checkchar);
   }
}

// This is not actually going to be called
// and is a placeholder for the main() function
int SystemTaskPlaceholder()
{
   while(1) { }
}

int ClockTask()
{
   while (1)
   {
      clk = ReadClock();
      milliseconds = ClockToMs(clk);
      ClockMsToHMS(milliseconds, hours, minutes, seconds);
   }

   return 0;
}

int MainTask()
{
   const uint8_t bgcolor = 0xC0; // BRG -> B=0xC0, R=0x38, G=0x07
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
   volatile unsigned int gpustate = 0x00000000;
   unsigned int cnt = 0x00000000;
   while(1)
   {
      // Hardware interrupt driven input processing happens async to this code

      if (have_a_break)
      {
         have_a_break = 0;
         asm volatile("ebreak;");
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

         if (showtime)
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

void SetupTasks()
{
   // Task 0 is a placeholder for the main() function
   // since the first time around we arrive at the timer interrupt
   // the PC/SP and registers belong to main()'s infinite spin loop.
   // Very short task (since it's a spinloop by default)
   task_array[0].PC = (uint32_t)SystemTaskPlaceholder;
   asm volatile("sw sp, %0;" : "=m" (task_array[0].SP) ); // Use current stack pointer of incoming call site
   task_array[0].quantum = 250; // run for 0.025ms then switch

   // Main application body, will be time-sliced (medium size task)
   task_array[1].PC = (uint32_t)MainTask;
   task_array[1].SP = 0x0003E000; // NOTE: Need a stack allocator for real addresses
   task_array[1].quantum = 10000; // run for 1ms then switch

   // Clock task (helps with time display and cursor blink, very short task)
   task_array[2].PC = (uint32_t)ClockTask;
   task_array[2].SP = 0x0003F000; // NOTE: Need a stack allocator for real addresses
   task_array[2].quantum = 2500; // run for 0.25ms then switch

   num_tasks = 3;
}

/*extern "C"
{*/
   void timer_interrupt()
   {
      // TODO: Save/Restore float registers

      // Save current task's registers (all except TP and GP)
      asm volatile("lw tp, 144(sp); sw tp, %0;" : "=m" (task_array[current_task].SP) );
      asm volatile("lw tp, 140(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[0]) );
      asm volatile("lw tp, 136(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[1]) );
      asm volatile("lw tp, 132(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[2]) );
      asm volatile("lw tp, 128(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[3]) );
      asm volatile("lw tp, 124(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[4]) );
      asm volatile("lw tp, 120(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[5]) );
      asm volatile("lw tp, 116(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[6]) );
      asm volatile("lw tp, 112(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[7]) );
      asm volatile("lw tp, 108(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[8]) );
      asm volatile("lw tp, 104(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[9]) );
      asm volatile("lw tp, 100(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[10]) );
      asm volatile("lw tp, 96(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[11]) );
      asm volatile("lw tp, 92(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[12]) );
      asm volatile("lw tp, 88(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[13]) );
      asm volatile("lw tp, 84(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[14]) );
      asm volatile("lw tp, 80(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[15]) );

      asm volatile("lw tp, 76(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[16]) );
      asm volatile("lw tp, 72(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[17]) );
      asm volatile("lw tp, 68(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[18]) );
      asm volatile("lw tp, 64(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[19]) );
      asm volatile("lw tp, 60(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[20]) );
      asm volatile("lw tp, 56(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[21]) );
      asm volatile("lw tp, 52(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[22]) );
      asm volatile("lw tp, 48(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[23]) );
      asm volatile("lw tp, 44(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[24]) );
      asm volatile("lw tp, 40(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[25]) );
      asm volatile("lw tp, 36(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[26]) );
      asm volatile("lw tp, 32(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[27]) );
      asm volatile("lw tp, 28(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[28]) );

      // Save the mret address of incoming call site
      asm volatile("csrr tp, mepc; sw tp, %0;" : "=m" (task_array[current_task].PC) );

      current_task= (current_task+1)%num_tasks;

      // Restore new task's registers (all except TP and GP)
      asm volatile("lw tp, %0; sw tp, 144(sp);" : : "m" (task_array[current_task].SP) );
      asm volatile("lw tp, %0; sw tp, 140(sp);" : : "m" (task_array[current_task].reg[0]) );
      asm volatile("lw tp, %0; sw tp, 136(sp);" : : "m" (task_array[current_task].reg[1]) );
      asm volatile("lw tp, %0; sw tp, 132(sp);" : : "m" (task_array[current_task].reg[2]) );
      asm volatile("lw tp, %0; sw tp, 128(sp);" : : "m" (task_array[current_task].reg[3]) );
      asm volatile("lw tp, %0; sw tp, 124(sp);" : : "m" (task_array[current_task].reg[4]) );
      asm volatile("lw tp, %0; sw tp, 120(sp);" : : "m" (task_array[current_task].reg[5]) );
      asm volatile("lw tp, %0; sw tp, 116(sp);" : : "m" (task_array[current_task].reg[6]) );
      asm volatile("lw tp, %0; sw tp, 112(sp);" : : "m" (task_array[current_task].reg[7]) );
      asm volatile("lw tp, %0; sw tp, 108(sp);" : : "m" (task_array[current_task].reg[8]) );
      asm volatile("lw tp, %0; sw tp, 104(sp);" : : "m" (task_array[current_task].reg[9]) );
      asm volatile("lw tp, %0; sw tp, 100(sp);" : : "m" (task_array[current_task].reg[10]) );
      asm volatile("lw tp, %0; sw tp, 96(sp);" : : "m" (task_array[current_task].reg[11]) );
      asm volatile("lw tp, %0; sw tp, 92(sp);" : : "m" (task_array[current_task].reg[12]) );
      asm volatile("lw tp, %0; sw tp, 88(sp);" : : "m" (task_array[current_task].reg[13]) );
      asm volatile("lw tp, %0; sw tp, 84(sp);" : : "m" (task_array[current_task].reg[14]) );
      asm volatile("lw tp, %0; sw tp, 80(sp);" : : "m" (task_array[current_task].reg[15]) );

      asm volatile("lw tp, %0; sw tp, 76(sp);" : : "m" (task_array[current_task].reg[16]) );
      asm volatile("lw tp, %0; sw tp, 72(sp);" : : "m" (task_array[current_task].reg[17]) );
      asm volatile("lw tp, %0; sw tp, 68(sp);" : : "m" (task_array[current_task].reg[18]) );
      asm volatile("lw tp, %0; sw tp, 64(sp);" : : "m" (task_array[current_task].reg[19]) );
      asm volatile("lw tp, %0; sw tp, 60(sp);" : : "m" (task_array[current_task].reg[20]) );
      asm volatile("lw tp, %0; sw tp, 56(sp);" : : "m" (task_array[current_task].reg[21]) );
      asm volatile("lw tp, %0; sw tp, 52(sp);" : : "m" (task_array[current_task].reg[22]) );
      asm volatile("lw tp, %0; sw tp, 48(sp);" : : "m" (task_array[current_task].reg[23]) );
      asm volatile("lw tp, %0; sw tp, 44(sp);" : : "m" (task_array[current_task].reg[24]) );
      asm volatile("lw tp, %0; sw tp, 40(sp);" : : "m" (task_array[current_task].reg[25]) );
      asm volatile("lw tp, %0; sw tp, 36(sp);" : : "m" (task_array[current_task].reg[26]) );
      asm volatile("lw tp, %0; sw tp, 32(sp);" : : "m" (task_array[current_task].reg[27]) );
      asm volatile("lw tp, %0; sw tp, 28(sp);" : : "m" (task_array[current_task].reg[28]) );

      // Set up new mret address to new tasks's saved (resume) PC
      asm volatile("lw tp, %0; csrrw zero, mepc, tp;" : : "m" (task_array[current_task].PC) );

      // Re-trigger the timer interrupt to trigger after task's run time
      uint32_t clockhigh, clocklow, tmp;
      asm volatile(
         "1:\n"
         "rdtimeh %0\n"
         "rdtime %1\n"
         "rdtimeh %2\n"
         "bne %0, %2, 1b\n"
         : "=&r" (clockhigh), "=&r" (clocklow), "=&r" (tmp)
      );
      uint64_t now = (uint64_t(clockhigh)<<32) | clocklow;
      uint64_t future = now + task_array[current_task].quantum; // Run time for next task minus tolerance
      asm volatile("csrrw zero, 0x801, %0" :: "r" ((future&0xFFFFFFFF00000000)>>32));
      asm volatile("csrrw zero, 0x800, %0" :: "r" (uint32_t(future&0x00000000FFFFFFFF)));
   }

   void breakpoint_interrupt()
   {
      gdb_stub();
      // This will continually fire when an ebreak is hit from a task.
      // Might be able to detect which call site the EBREAK is at and report it etc
      /*if (breakpoint_triggered == 0)
      {
         breakpoint_triggered = 1;
         uint32_t breakpointPC = 0;
         asm volatile("csrr tp, mepc; sw tp, %0;" : "=m" (breakpointPC) );

         EchoUART("Breakpoint triggered at ");
         EchoInt(breakpointPC);
         EchoUART(" instruction: ");
         EchoInt(saved_instruction);
         EchoUART("\r\n");
      }*/
   }

   void external_interrupt()
   {
      // TODO: GDB stub here
      //ProcessUARTInputAsync();
      gdb_stub();
   }
//}

//void __attribute__((interrupt("machine"))) interrupt_handler()
void __attribute__((naked)) interrupt_handler()
{
   asm volatile (
      "mv tp, sp;"
      "addi	sp,sp,-148;"
      "sw	tp,144(sp);"
      "sw	ra,140(sp);"
      "sw	t0,136(sp);"
      "sw	t1,132(sp);"
      "sw	t2,128(sp);"
      "sw	a0,124(sp);"
      "sw	a1,120(sp);"
      "sw	a2,116(sp);"
      "sw	a3,112(sp);"
      "sw	a4,108(sp);"
      "sw	a5,104(sp);"
      "sw	a6,100(sp);"
      "sw	a7,96(sp);"
      "sw	t3,92(sp);"
      "sw	t4,88(sp);"
      "sw	t5,84(sp);"
      "sw	t6,80(sp);"

      //"sw	gp,76(sp);" // Do not save/restore this within same executable
      "sw	s0,72(sp);"
      "sw	s1,68(sp);"
      "sw	s2,64(sp);"
      "sw	s3,60(sp);"
      "sw	s4,56(sp);"
      "sw	s5,52(sp);"
      "sw	s6,48(sp);"
      "sw	s7,44(sp);"
      "sw	s8,40(sp);"
      "sw	s9,36(sp);"
      "sw	s10,32(sp);"
      "sw	s11,28(sp);"

      /*"fsw	ft0,76(sp);" // TODO: FPU state later
      "fsw	ft1,72(sp);"
      "fsw	ft2,68(sp);"
      "fsw	ft3,64(sp);"
      "fsw	ft4,60(sp);"
      "fsw	ft5,56(sp);"
      "fsw	ft6,52(sp);"
      "fsw	ft7,48(sp);"
      "fsw	fa0,44(sp);"
      "fsw	fa1,40(sp);"
      "fsw	fa2,36(sp);"
      "fsw	fa3,32(sp);"
      "fsw	fa4,28(sp);"
      "fsw	fa5,24(sp);"
      "fsw	fa6,20(sp);"
      "fsw	fa7,16(sp);"
      "fsw	ft8,12(sp);"
      "fsw	ft9,8(sp);"
      "fsw	ft10,4(sp);"
      "fsw	ft11,0(sp);"*/
   );

   register uint32_t causedword;
   asm volatile("csrr %0, mcause" : "=r"(causedword));
   if (causedword==7) // Timer
      timer_interrupt();
   else if (causedword==3) // Breakpoint
      breakpoint_interrupt();
   else if (causedword==11) // External
      external_interrupt();

   asm volatile(
      "lw	tp,144(sp);"
      "lw	ra,140(sp);"
      "lw	t0,136(sp);"
      "lw	t1,132(sp);"
      "lw	t2,128(sp);"
      "lw	a0,124(sp);"
      "lw	a1,120(sp);"
      "lw	a2,116(sp);"
      "lw	a3,112(sp);"
      "lw	a4,108(sp);"
      "lw	a5,104(sp);"
      "lw	a6,100(sp);"
      "lw	a7,96(sp);"
      "lw	t3,92(sp);"
      "lw	t4,88(sp);"
      "lw	t5,84(sp);"
      "lw	t6,80(sp);"

      //"lw	gp,76(sp);"
      "lw	s0,72(sp);"
      "lw	s1,68(sp);"
      "lw	s2,64(sp);"
      "lw	s3,60(sp);"
      "lw	s4,56(sp);"
      "lw	s5,52(sp);"
      "lw	s6,48(sp);"
      "lw	s7,44(sp);"
      "lw	s8,40(sp);"
      "lw	s9,36(sp);"
      "lw	s10,32(sp);"
      "lw	s11,28(sp);"

      /*"flw	ft0,76(sp);"
      "flw	ft1,72(sp);"
      "flw	ft2,68(sp);"
      "flw	ft3,64(sp);"
      "flw	ft4,60(sp);"
      "flw	ft5,56(sp);"
      "flw	ft6,52(sp);"
      "flw	ft7,48(sp);"
      "flw	fa0,44(sp);"
      "flw	fa1,40(sp);"
      "flw	fa2,36(sp);"
      "flw	fa3,32(sp);"
      "flw	fa4,28(sp);"
      "flw	fa5,24(sp);"
      "flw	fa6,20(sp);"
      "flw	fa7,16(sp);"
      "flw	ft8,12(sp);"
      "flw	ft9,8(sp);"
      "flw	ft10,4(sp);"
      "flw	ft11,0(sp);"*/
      "mv	sp,tp;" // Use the 'restored' stack pointer (timer_interrupt might have modified it)
      //"addi	sp,sp,148;"
      "mret;" // May return to a different call site if timer_interrupt switched tasks
   );
}

void SetupInterruptHandlers()
{
   // Set up timer interrupt one second into the future
   // NOTE: timecmp register is a custom register on NekoIchi, not memory mapped
   // which would not make sense since memory mapping would have delays and
   // would interfere with how NekoIchi accesses memory
   // NOTE: ALWAYS set high part first to avoid false triggers
   uint32_t clockhigh, clocklow, tmp;
   asm volatile(
      "1:\n"
      "rdtimeh %0\n"
      "rdtime %1\n"
      "rdtimeh %2\n"
      "bne %0, %2, 1b\n"
      : "=&r" (clockhigh), "=&r" (clocklow), "=&r" (tmp)
   );
   uint64_t now = (uint64_t(clockhigh)<<32) | clocklow;
   uint64_t future = now + DEFAULT_TIMESLICE; // Entry point spins for this amount initially before switching
   asm volatile("csrrw zero, 0x801, %0" :: "r" ((future&0xFFFFFFFF00000000)>>32));
   asm volatile("csrrw zero, 0x800, %0" :: "r" (uint32_t(future&0x00000000FFFFFFFF)));

   // Set trap handler address
   asm volatile("csrrw zero, mtvec, %0" :: "r" (interrupt_handler));

   // Enable machine interrupts
   int mstatus = (1 << 3);
   asm volatile("csrrw zero, mstatus,%0" :: "r" (mstatus));

   // Enable machine timer interrupts, machine external interrupts and debug interrupt
   int msie = (1 << 7) | (1 << 11) | (1 << 3);
   asm volatile("csrrw zero, mie,%0" :: "r" (msie));
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
   if (LoadELF("BOOT.ELF") != -1)
       RunELF();

   SetupTasks();
   SetupInterruptHandlers();

   //set_debug_traps();
   //breakpoint();

   // This loop is a bit iffy, better not add code in here
   while (1) { }

   return 0;
}