// Embedded utils

#include <stdlib.h>
#include <errno.h>

#include "core.h"
#include "uart.h"

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
