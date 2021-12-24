// Embedded utils

#include <stdlib.h>
#include <errno.h>

#include "core.h"
#include "uart.h"

// Utilities

uint64_t E32ReadTime()
{
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
   return now;
}

void E32SetTimeCompare(const uint64_t future)
{
   // NOTE: ALWAYS set high word first to avoid misfires outside timer interrupt
   swap_csr(0x801, ((future&0xFFFFFFFF00000000)>>32));
   swap_csr(0x800, (uint32_t(future&0x00000000FFFFFFFF)));
}

// C stdlib overrides

// Place the heap into DDR3 memory
//#undef errno
//int nerrno;
static uint8_t *heap_start  = (uint8_t*)0x00008000;
static uint8_t *heap_end    = (uint8_t*)0x0000F000; // Leave 0xFFF for stack

extern "C" {

   /*void unimplemented_syscall()
   {
      asm volatile ("ebreak");
      __builtin_unreachable();
   }*/

   int _brk(void *addr)
   {
      heap_start = (uint8_t*)addr;
      return 0;
   }

   void *_sbrk(intptr_t incr)
   {
      uint8_t *old_heapstart = heap_start;

      if (heap_start == heap_end) {
         return NULL;
      }

      if ((heap_start += incr) < heap_end) {
         heap_start += incr;
      } else {
         heap_start = heap_end;
      }
      return old_heapstart;
   }

   ssize_t _read(int file, void *ptr, size_t len)
   {
      return 0; // eof
   }

   ssize_t _write(int file, const void *ptr, size_t len)
   {
      if(file == 1) // STDIO
      {
         for(size_t i=0; i<len; ++i)
            UARTPutChar(((char*)ptr)[i]);
      }

      return 0;
   }

   int _close(int file)
   {
      // close is called before _exit()
      return 0;
   }

   int _fstat(int file, struct stat *st)
   {
      // fstat is called during libc startup
      errno = ENOENT;
      return -1;
   }
}

/*
_open
_openat
_lseek
_stat
_lstat
_fstatat
_isatty
_access
_faccessat
_link
_unlink
_execve
_getpid
_fork
_kill
_wait
_times
_gettimeofday
_ftime
_utime
_chown
_chmod
_chdir
_getcwd
_sysconf
*/
