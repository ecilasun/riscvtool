// NekoIchi ROM
// Experimental

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <unistd.h>
#include "nekoichi.h"
#include "gpu.h"
#include "sdcard.h"
#include "console.h"
#include "elf.h"
#include "nekoichitask.h"
#include "debugger.h"
#include "../fat32/ff.h"

// NOTE: Uncomment when experimental ROM should be compiled as actual ROM image
// Also need to swap the the ROM image compile command in build.sh file
//#define ROM_STARTUP_128K
//#include "rom_nekoichi_rvcrt0.h"

// Entry zero will always be main()
uint32_t current_task = 0; // init_task
cpu_context task_array[MAX_TASKS];
uint32_t num_tasks = 0; // only one initially, which is the init_task
uint32_t sdcardstate;
uint32_t sdcardtriggerread;
uint32_t recessmaintask = 0;
uint32_t eventuallydie = 0;

// States
uint8_t oldhardwareswitchstates = 0;
uint8_t hardwareswitchstates = 0;

FATFS Fs;
uint32_t sdcardavailable = 0;
volatile uint32_t branchaddress = 0x10000; // TODO: Branch to actual entry point
const char *FRtoString[]={
	"Succeeded\r\n",
	"A hard error occurred in the low level disk I/O layer\r\n",
	"Assertion failed\r\n",
	"The physical drive cannot work\r\n",
	"Could not find the file\r\n",
	"Could not find the path\r\n",
	"The path name format is invalid\r\n",
	"Access denied due to prohibited access or directory full\r\n",
	"Access denied due to prohibited access\r\n",
	"The file/directory object is invalid\r\n",
	"The physical drive is write protected\r\n",
	"The logical drive number is invalid\r\n",
	"The volume has no work area\r\n",
	"There is no valid FAT volume\r\n",
	"The f_mkfs() aborted due to any problem\r\n",
	"Could not get a grant to access the volume within defined period\r\n",
	"The operation is rejected according to the file sharing policy\r\n",
	"LFN working buffer could not be allocated\r\n",
	"Number of open files > FF_FS_LOCK\r\n",
	"Given parameter is invalid\r\n"
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

void ParseELFHeaderAndLoadSections(FILE *fp, SElfFileHeader32 *fheader, const uint32_t offset)
{
   if (fheader->m_Magic != 0x464C457F)
   {
       printf("Failed: expecting 0x7F+'ELF'\n");
       return;
   }

   branchaddress = offset + fheader->m_Entry;

   // Read program header
   SElfProgramHeader32 pheader;
   fseek(fp, fheader->m_PHOff, SEEK_SET);
   fread(&pheader, sizeof(SElfProgramHeader32), 1, fp);

   // Read string table section header
   unsigned int stringtableindex = fheader->m_SHStrndx;
   SElfSectionHeader32 stringtablesection;
   fseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex, SEEK_SET);
   fread(&stringtablesection, sizeof(SElfSectionHeader32), 1, fp);

   // Allocate memory for and read string table
   char *names = (char *)malloc(stringtablesection.m_Size);
   fseek(fp, stringtablesection.m_Offset, SEEK_SET);
   fread(names, stringtablesection.m_Size, 1, fp);

   // Load all loadable sections
   for(unsigned short i=0; i<fheader->m_SHNum; ++i)
   {
      // Seek-load section headers as needed
      SElfSectionHeader32 sheader;
      fseek(fp, fheader->m_SHOff+fheader->m_SHEntSize*i, SEEK_SET);
      fread(&sheader, sizeof(SElfSectionHeader32), 1, fp);

      // If this is a section worth loading...
      if (sheader.m_Flags & 0x00000007 && sheader.m_Size!=0)
      {
         // ...place it in memory
         uint8_t *elfsectionpointer = (uint8_t *)sheader.m_Addr + offset;
         fseek(fp, sheader.m_Offset, SEEK_SET);
         printf("Loading section '%s' @0x%.8X, %.8X bytes\r\n", names+sheader.m_NameOffset, (unsigned int)elfsectionpointer, sheader.m_Size);
         fread(elfsectionpointer, sheader.m_Size, 1, fp);
      }
   }

   free(names);
   printf("Done.\r\n");
}

void LoadFile(char *filename, uint32_t base)
{
   int percent = 0;
   FILE *fp = fopen(filename, "rb");
   if (fp != nullptr)
   {
      fseek(fp, 0, SEEK_END);
      long size = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      long chunks = (size+511)/512;
      printf("%d bytes\r\n", (unsigned int)size);
      for (int c=0;c<chunks;++c)
      {
         printf("\0337 Loading [");
         for (int i=0;i<percent;++i)
            printf(":");
         for (int i=0;i<20-percent;++i)
            printf(" ");
         printf("]  %d bytes\0338", 512*c);
         fread((void*)(base+c*512), 512, 1, fp);
         percent = (c*20)/chunks;
      }
      printf("File loaded at 0x%.8X                      \r\n", (unsigned int)base);

      fclose(fp);
   }
   else
      printf("File %s not found\r\n", filename);
}

void Dump(uint32_t base, uint32_t offset)
{
   char outstr[40];
   char str[9];
   str[8]=0;
   for(uint32_t i=0;i<16;i+=2) // Fs.fsize
   {
      uint32_t V1 = *(uint32_t*)(base+4*offset+4*(i+0));
      uint32_t V2 = *(uint32_t*)(base+4*offset+4*(i+1));
      str[0] = V1&0x000000FF; str[0] = ((str[0]<32) | (str[0]>127)) ? '.':str[0];
      str[1] = (V1>>8)&0x000000FF; str[1] = ((str[1]<32) | (str[1]>127)) ? '.':str[1];
      str[2] = (V1>>16)&0x000000FF; str[2] = ((str[2]<32) | (str[2]>127)) ? '.':str[2];
      str[3] = (V1>>24)&0x000000FF; str[3] = ((str[3]<32) | (str[3]>127)) ? '.':str[3];
      str[4] = V2&0x000000FF; str[4] = ((str[4]<32) | (str[4]>127)) ? '.':str[4];
      str[5] = (V2>>8)&0x000000FF; str[5] = ((str[5]<32) | (str[5]>127)) ? '.':str[5];
      str[6] = (V2>>16)&0x000000FF; str[6] = ((str[6]<32) | (str[6]>127)) ? '.':str[6];
      str[7] = (V2>>24)&0x000000FF; str[7] = ((str[7]<32) | (str[7]>127)) ? '.':str[7];
      sprintf(outstr, "%.8X%.8X: %s\r\n", (unsigned int)V1,(unsigned int)V2, str);
      printf(outstr);
      EchoConsole(outstr);
   }
}

int LoadELF(const char *filename)
{
   const uint32_t offset = 0x00000000; // To be used during debug
   FILE *fp = fopen(filename, "rb");
   if (fp != nullptr)
   {
      // File size: Fs.fsize
      // Read header
      SElfFileHeader32 fheader;
      fread(&fheader, sizeof(fheader), 1, fp);
      ParseELFHeaderAndLoadSections(fp, &fheader, offset);
      fclose(fp);
      return 0;
   }
   else
   {
      printf("File '%s' not found\r\n", filename);
      return -1;
   }
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
            int fidx=0;
            char flags[64]="";
            if (finf.fattrib&0x01) flags[fidx++]='r';
            if (finf.fattrib&0x02) flags[fidx++]='h';
            if (finf.fattrib&0x04) flags[fidx++]='s';
            if (finf.fattrib&0x08) flags[fidx++]='l';
            if (finf.fattrib&0x0F) flags[fidx++]='L';
            if (finf.fattrib&0x10) flags[fidx++]='d';
            if (finf.fattrib&0x20) flags[fidx++]='a';
            flags[fidx++]=0;
            printf("%s %d %s\r\n", finf.fname, (int)finf.fsize, flags);
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      printf("%s", FRtoString[re]);
}

void HandleDemoCommands(char checkchar)
{
   static uint32_t doffset = 0;

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
      printf("\r\n");

      // Clear the whole screen
      if ((cmdbuffer[0]=='c') && (cmdbuffer[1]=='l') && (cmdbuffer[2]=='s'))
      {
         ClearConsole();
      }
      else if ((cmdbuffer[0]=='h') && (cmdbuffer[1]=='e') && (cmdbuffer[2]=='l') && (cmdbuffer[3]=='p'))
      {
         EchoConsole("dir: list files\r\n");
         EchoConsole("run filename: load and run ELF\n\r");
         EchoConsole("load filename: load at 0x0A000000\n\r");
         EchoConsole("dump: dump at 0x0A000000 & inc offset\n\r");
         EchoConsole("cls: clear screen\r\n");
         EchoConsole("help: help screen\r\n");
         EchoConsole("time: toggle time\r\n");
         EchoConsole("die: run illegal instruction\r\n");
      }
      else if ((cmdbuffer[0]=='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='r'))
      {
         ListDir(&cmdbuffer[4]);
      }
      else if ((cmdbuffer[0]=='d') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='e'))
      {
         eventuallydie = 1;
      }
      else if ((cmdbuffer[0]=='r') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='n'))
      {
         if (LoadELF(&cmdbuffer[4]) != -1) // Skip 'run ' part
         {
            task_array[3].PC = branchaddress;
            num_tasks = 4; // Next time, scheduler will consider the loaded task as well
            recessmaintask = 1;
         }
      }
      else if ((cmdbuffer[0]=='l') && (cmdbuffer[1]=='o') && (cmdbuffer[2]=='a') && (cmdbuffer[3]=='d'))
      {
         doffset = 0;
         LoadFile(&cmdbuffer[5], 0x0A000000); // Skip 'load ' part
      }
      else if ((cmdbuffer[0]=='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
      {
         Dump(0x0A000000, doffset);
         doffset += 16;
      }
      else if ((cmdbuffer[0]=='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
         showtime = (showtime+1)%2;
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

// OS Idle task
int OSIdleTask()
{
   while(1) { }
}

int OSPeriodicTask()
{
   uint32_t lastmilliseconds = 0;
   while (1)
   {
      clk = ReadClock();
      milliseconds = ClockToMs(clk);
      ClockMsToHMS(milliseconds, hours, minutes, seconds);

      if (milliseconds - lastmilliseconds > 200)
      {
         lastmilliseconds = milliseconds;

         uint32_t newsdcardstate = (hardwareswitchstates&0x80) ? 0x00 : 0x01;
         if (newsdcardstate != sdcardstate)
         {
            // Trigger for this to happen in the main loop
            //if (newsdcardstate) -> Triggers only when card inserted
               sdcardtriggerread = 1;
            sdcardstate = newsdcardstate;
         }
      }
   }

   return 0;
}

int OSMainTask()
{
   uint8_t bgcolor = 0x1A; // Light gray
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
   // Make sure this lands in the Fast RAM
   volatile uint32_t *gpustate = (volatile uint32_t *)0x00005000;
   *gpustate = 0x0;
   unsigned int cnt = 0x00000000;
   while(1)
   {
      // Main task should not interfere with graphics etc if
      // an ELF was loaded
      if (recessmaintask)
         continue;

      // Hardware interrupt driven input processing happens async to this code

      if (sdcardtriggerread)
      {
         sdcardtriggerread = 0;
         FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
         if (mountattempt!=FR_OK)
            printf(FRtoString[mountattempt]);
         /*else
         {
            FRESULT chdirattempt = f_chdir("sd:/");
            if (FR_OK != chdirattempt)
               printf(FRtoString[chdirattempt]);
         }*/
         sdcardavailable = ( mountattempt == FR_OK) ? 1 : 0;
         printf(sdcardavailable ? "SDCard available\r\n" : "SDCard not available\r\n");
         if (sdcardavailable && LoadELF("BOOT.ELF") != -1)
         {
            task_array[3].PC = branchaddress;
            num_tasks = 4; // Next time, scheduler will consider the loaded task as well
            recessmaintask = 1;
         }
      }

      if (eventuallydie)
      {
         eventuallydie = 0;

         printf("Crashing...\r\n");
         // Generate illegal instructions
         asm volatile(".dword 0x00000000");
         asm volatile(".dword 0xFFFFFFFF");
      }

      if (*gpustate == cnt) // GPU work complete, push more
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
         uint32_t gpustateDWORDaligned = uint32_t(gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteSystemMemory(2, 1);

         *gpustate = 0;
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
   task_array[0].name = "OSIdle";
   task_array[0].PC = (uint32_t)OSIdleTask;
   asm volatile("sw sp, %0;" : "=m" (task_array[0].reg[2]) ); // Use current stack pointer of incoming call site
   asm volatile("sw s0, %0;" : "=m" (task_array[0].reg[8]) ); // Use current frame pointer of incoming call site
   task_array[0].quantum = 250; // run for 0.025ms then switch

   // Main application body, will be time-sliced (medium size task)
   task_array[1].name = "OSMain";
   task_array[1].PC = (uint32_t)OSMainTask;
   task_array[1].reg[2] = 0x0003F000; // NOTE: Need a stack allocator for real addresses
   task_array[1].reg[8] = 0x0003F000; // Frame pointer
   task_array[1].quantum = 5000; // run for 0.5ms then switch

   // Clock task (helps with time display and cursor blink, very short task)
   task_array[2].name = "OSPeriodic";
   task_array[2].PC = (uint32_t)OSPeriodicTask;
   task_array[2].reg[2] = 0x0003E010; // NOTE: Need a stack allocator for real addresses
   task_array[2].reg[8] = 0x0003E010; // Frame pointer
   task_array[2].quantum = 1000; // run for 0.1ms then switch

   // User task placeholder for code loaded from storage
   task_array[3].name = "UserTask";
   task_array[3].PC = (uint32_t)0x10000;
   task_array[3].reg[2] = 0x0003C030; // NOTE: ELF needs a stack pointer set up
   task_array[3].reg[8] = 0x0003C030; // NOTE: ELF needs a frame set up
   task_array[3].quantum = 100000; // run for 10ms then switch to OS tasks

   num_tasks = 3; // Ignore user task until something is loaded and the entry point is patched, set to 4 afterwards
}

void timer_interrupt()
{
   // TODO: Save/Restore float registers

   // Save current task's registers (all except TP and GP)
   asm volatile("lw tp, 144(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[1]) );
   asm volatile("lw tp, 140(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[2]) ); // New stack pointer
   asm volatile("lw tp, 136(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[5]) );
   asm volatile("lw tp, 132(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[6]) );
   asm volatile("lw tp, 128(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[7]) );
   asm volatile("lw tp, 124(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[10]) );
   asm volatile("lw tp, 120(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[11]) );
   asm volatile("lw tp, 116(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[12]) );
   asm volatile("lw tp, 112(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[13]) );
   asm volatile("lw tp, 108(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[14]) );
   asm volatile("lw tp, 104(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[15]) );
   asm volatile("lw tp, 100(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[16]) );
   asm volatile("lw tp, 96(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[17]) );
   asm volatile("lw tp, 92(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[28]) );
   asm volatile("lw tp, 88(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[29]) );
   asm volatile("lw tp, 84(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[30]) );
   asm volatile("lw tp, 80(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[31]) );
   //asm volatile("lw tp, 76(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[3]) );
   asm volatile("lw tp, 72(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[8]) ); // Old stack pointer
   asm volatile("lw tp, 68(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[9]) );
   asm volatile("lw tp, 64(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[18]) );
   asm volatile("lw tp, 60(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[19]) );
   asm volatile("lw tp, 56(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[20]) );
   asm volatile("lw tp, 52(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[21]) );
   asm volatile("lw tp, 48(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[22]) );
   asm volatile("lw tp, 44(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[23]) );
   asm volatile("lw tp, 40(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[24]) );
   asm volatile("lw tp, 36(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[25]) );
   asm volatile("lw tp, 32(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[26]) );
   asm volatile("lw tp, 28(sp); sw tp, %0;" : "=m" (task_array[current_task].reg[27]) );

   // Save the mret address of incoming call site
   asm volatile("csrr tp, mepc; sw tp, %0;" : "=m" (task_array[current_task].PC) );

   // Break
   if (task_array[current_task].ctrlc == 1)
   {
      task_array[current_task].ctrlc = 2;
      task_array[current_task].ctrlcaddress = task_array[current_task].PC;
      task_array[current_task].ctrlcbackup = *(uint32_t*)task_array[current_task].PC;
      *(uint32_t*)task_array[current_task].PC = 0x00100073; // EBREAK
   }

   // Resume
   if (task_array[current_task].ctrlc == 8)
   {
      task_array[current_task].breakhit = 0;
      task_array[current_task].ctrlc = 0;
      *(uint32_t*)(task_array[current_task].ctrlcaddress) = task_array[current_task].ctrlcbackup;
   }

   // Step
   /*if (task_array[current_task].ctrlc == 16)
   {
      task_array[current_task].ctrlc = 2;
      *(uint32_t*)(task_array[current_task].ctrlcaddress) = task_array[current_task].ctrlcbackup;
      // NOTE: if the next address is a jump target, how do I know where to step?
      task_array[current_task].ctrlcaddress = task_array[current_task].PC+4;
      task_array[current_task].ctrlcbackup = *(uint32_t*)(task_array[current_task].PC+4);
      *(uint32_t*)(task_array[current_task].PC+4) = 0x00100073; // EBREAK
   }*/

   current_task= (current_task+1)%num_tasks;

   // Restore new task's registers (all except TP and GP)
   asm volatile("lw tp, %0; sw tp, 144(sp);" : : "m" (task_array[current_task].reg[1]) );
   asm volatile("lw tp, %0; sw tp, 140(sp);" : : "m" (task_array[current_task].reg[2]) );
   asm volatile("lw tp, %0; sw tp, 136(sp);" : : "m" (task_array[current_task].reg[5]) );
   asm volatile("lw tp, %0; sw tp, 132(sp);" : : "m" (task_array[current_task].reg[6]) );
   asm volatile("lw tp, %0; sw tp, 128(sp);" : : "m" (task_array[current_task].reg[7]) );
   asm volatile("lw tp, %0; sw tp, 124(sp);" : : "m" (task_array[current_task].reg[10]) );
   asm volatile("lw tp, %0; sw tp, 120(sp);" : : "m" (task_array[current_task].reg[11]) );
   asm volatile("lw tp, %0; sw tp, 116(sp);" : : "m" (task_array[current_task].reg[12]) );
   asm volatile("lw tp, %0; sw tp, 112(sp);" : : "m" (task_array[current_task].reg[13]) );
   asm volatile("lw tp, %0; sw tp, 108(sp);" : : "m" (task_array[current_task].reg[14]) );
   asm volatile("lw tp, %0; sw tp, 104(sp);" : : "m" (task_array[current_task].reg[15]) );
   asm volatile("lw tp, %0; sw tp, 100(sp);" : : "m" (task_array[current_task].reg[16]) );
   asm volatile("lw tp, %0; sw tp, 96(sp);" : : "m" (task_array[current_task].reg[17]) );
   asm volatile("lw tp, %0; sw tp, 92(sp);" : : "m" (task_array[current_task].reg[28]) );
   asm volatile("lw tp, %0; sw tp, 88(sp);" : : "m" (task_array[current_task].reg[29]) );
   asm volatile("lw tp, %0; sw tp, 84(sp);" : : "m" (task_array[current_task].reg[30]) );
   asm volatile("lw tp, %0; sw tp, 80(sp);" : : "m" (task_array[current_task].reg[31]) );
   //asm volatile("lw tp, %0; sw tp, 76(sp);" : : "m" (task_array[current_task].reg[3]) );
   asm volatile("lw tp, %0; sw tp, 72(sp);" : : "m" (task_array[current_task].reg[8]) );
   asm volatile("lw tp, %0; sw tp, 68(sp);" : : "m" (task_array[current_task].reg[9]) );
   asm volatile("lw tp, %0; sw tp, 64(sp);" : : "m" (task_array[current_task].reg[18]) );
   asm volatile("lw tp, %0; sw tp, 60(sp);" : : "m" (task_array[current_task].reg[19]) );
   asm volatile("lw tp, %0; sw tp, 56(sp);" : : "m" (task_array[current_task].reg[20]) );
   asm volatile("lw tp, %0; sw tp, 52(sp);" : : "m" (task_array[current_task].reg[21]) );
   asm volatile("lw tp, %0; sw tp, 48(sp);" : : "m" (task_array[current_task].reg[22]) );
   asm volatile("lw tp, %0; sw tp, 44(sp);" : : "m" (task_array[current_task].reg[23]) );
   asm volatile("lw tp, %0; sw tp, 40(sp);" : : "m" (task_array[current_task].reg[24]) );
   asm volatile("lw tp, %0; sw tp, 36(sp);" : : "m" (task_array[current_task].reg[25]) );
   asm volatile("lw tp, %0; sw tp, 32(sp);" : : "m" (task_array[current_task].reg[26]) );
   asm volatile("lw tp, %0; sw tp, 28(sp);" : : "m" (task_array[current_task].reg[27]) );

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

void illegalinstruction_exception()
{
   register uint32_t at;
   asm volatile("csrr %0, mtval" : "=r"(at));

   printf("Illegal instruction @0x%.8X: 0x%.8X\r\n", (unsigned int)at, *(unsigned int*)at);

   // Cause a debug breakpoint (but the place it breaks might be wrong for now)
   //gdb_breakpoint(task_array);

   // Deadlock
   while(1) { }
}

void breakpoint_interrupt()
{
   // Notify GDB about having hit a breakpoint so it knows we've paused
   gdb_breakpoint(task_array);
}

void external_interrupt(uint32_t deviceID)
{
   // This handler will service all hardware interrupts simultaneusly

   if (deviceID&0x0001) // UART
   {
      if (hardwareswitchstates&0x01) // First slide switch controls debug mode
         gdb_handler(task_array, num_tasks);
      else
         ProcessUARTInputAsync();
   }

   if (deviceID&0x0002) // SWITCHES
   {
      // Handle switch interaction, possibly stash switch state to a pre-determined memory address from mscratch register
      hardwareswitchstates = *IO_SwitchState;

      // Debug output for individual switches that changed state
      int osbutton = 0;
      switch(hardwareswitchstates ^ oldhardwareswitchstates)
      {
         case 0x01: printf(hardwareswitchstates&0x01 ? "GDBServer\r\n" : "Terminal\r\n"); break;
         case 0x02: printf(hardwareswitchstates&0x02 ? "S1HIGH\r\n" : "S1LOW\r\n"); break;
         case 0x04: printf(hardwareswitchstates&0x04 ? "S2HIGH\r\n" : "S2LOW\r\n"); break;
         case 0x08: printf(hardwareswitchstates&0x08 ? "S3HIGH\r\n" : "S3LOW\r\n"); break;
         case 0x10: printf(hardwareswitchstates&0x10 ? "...\r\n" : "Toggle OSMain\r\n"); osbutton = 1; break;
         case 0x20: printf(hardwareswitchstates&0x20 ? "B1DOWN\r\n" : "B1UP\r\n"); break;
         case 0x40: printf(hardwareswitchstates&0x40 ? "B2DOWN\r\n" : "B2UP\r\n"); break;
         //case 0x80: printf(hardwareswitchstates&0x80 ? "SDREMOVED\r\n" : "SDINSERTED\r\n"); break;
         default: break;
      };

      // Toggle main vs loaded app
      if (osbutton && ((hardwareswitchstates&0x10) == 0)) // OS button up
      {
         num_tasks = recessmaintask ? 3 : 4;
         recessmaintask = !recessmaintask;
      }

      oldhardwareswitchstates = hardwareswitchstates;
   }
}

//void __attribute__((interrupt("machine"))) interrupt_handler()
void __attribute__((naked)) interrupt_handler()
{
   asm volatile (
      "mv tp, sp;"
      "addi	sp,sp,-148;"
      "sw	ra,144(sp);"
      "sw	tp,140(sp);"
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
   switch(causedword&0x0000FFFF)
   {
      case 2: // Illegal instruction
         illegalinstruction_exception();
         break;
      case 3: // Breakpoint
         breakpoint_interrupt();
         break;
      case 7: // Timer
         timer_interrupt();
         break;
      case 11: // External
         // NOTE: Highest bit is set when the cause is an interrupt
         external_interrupt((causedword&0x7FFF0000)>>16);
         break;
      default:
         // NOTE: Unknown interrupts will be ignored
         break;
   }

   // NOTE: The stack values corresponding to these registers
   // will be overwritten by the task switcher if a timer
   // interrupt is hit and it schedules another task,
   // as well as modifying the return address for mret, so that
   // we resume from within another function when we return.
   asm volatile(
      "lw	ra,144(sp);" // 1
      "lw	tp,140(sp);" // 2 (alias) -> Stack pointer
      "lw	t0,136(sp);" // 5
      "lw	t1,132(sp);" // 6
      "lw	t2,128(sp);" // 7
      "lw	a0,124(sp);" // 10
      "lw	a1,120(sp);" // 11
      "lw	a2,116(sp);" // 12
      "lw	a3,112(sp);" // 13
      "lw	a4,108(sp);" // 14
      "lw	a5,104(sp);" // 15
      "lw	a6,100(sp);" // 16
      "lw	a7,96(sp);"  // 17
      "lw	t3,92(sp);"  // 28
      "lw	t4,88(sp);"  // 29
      "lw	t5,84(sp);"  // 30
      "lw	t6,80(sp);"  // 31
      //"lw	gp,76(sp);"  // 3 <<<
      "lw	s0,72(sp);"  // 8 (fp) -> Stack frame (point at previous stack pointer)
      "lw	s1,68(sp);"  // 9
      "lw	s2,64(sp);"  // 18
      "lw	s3,60(sp);"  // 19
      "lw	s4,56(sp);"  // 20
      "lw	s5,52(sp);"  // 21
      "lw	s6,48(sp);"  // 22
      "lw	s7,44(sp);"  // 23
      "lw	s8,40(sp);"  // 24
      "lw	s9,36(sp);"  // 25
      "lw	s10,32(sp);" // 26
      "lw	s11,28(sp);" // 27
      //"lw	tp,...(sp);"  // 4 actual TP is lost (overwritten by SP)

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

   // Enable machine timer interrupts, machine external interrupts, debug interrupt and illegal instruction exception
   int msie = (1 << 7) | (1 << 11) | (1 << 3) | (1 << 2);
   asm volatile("csrrw zero, mie,%0" :: "r" (msie));

   // Enable machine interrupts
   int mstatus = (1 << 3);
   asm volatile("csrrw zero, mstatus,%0" :: "r" (mstatus));
}

int main()
{
   setbuf(stdout, NULL);
   InitFont();

   // 'clear' terminal and print the RV logo
   printf("\033[2J\r\n");
   printf("+-------------------------+\r\n");
   printf("|          ************** |\r\n");
   printf("| ########   ************ |\r\n");
   printf("| #########  ************ |\r\n");
   printf("| ########   ***********  |\r\n");
   printf("| #        ***********    |\r\n");
   printf("| ##   *************   ## |\r\n");
   printf("| ####   *********   #### |\r\n");
   printf("| ######   *****   ###### |\r\n");
   printf("| ########   *   ######## |\r\n");
   printf("| ##########   ########## |\r\n");
   printf("+-------------------------+\r\n");

   // Grab MISA register and show type of machine we're running on
   register uint32_t ISAword;
   asm volatile("csrr %0, misa" : "=r"(ISAword));
   printf("\r\nRV32IMFZicsr (0x%.8X)\r\n", (unsigned int)ISAword); //{2'b01, 4'b0000, 26'b00000000000001000100100000}; -> 32bit I M F

   // Copyright message
   printf("NekoIchi \u00A9 2021 Engin Cilasun\r\n");

   // Grab the initial state of switches
   // This read does not trigger an interrupt but reads the live state
   // Reads after an interrupt come from switch state FIFO
   hardwareswitchstates = oldhardwareswitchstates = *IO_SwitchState;
   sdcardstate = 0xFF;

   SetupTasks();
   SetupInterruptHandlers();

   // If main() is not listed in the task list,
   // this while loop will only run slightly shorter than DEFAULT_TIMESLICE.
   // Therefore, it can only contain very short sequence of instructions (about 1ms),
   // after which the task scheduler will never visit it.
   // In the case where SetupTasks/SetupInterruptHandlers are invoked
   // from _start(), main() can live in the task list
   // and will be re-visited as often as possible in which case
   // it can replace the dummy OSIdle task.
   while (1) { }

   return 0;
}
