// Embedded utils

#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "core.h"
#include "uart.h"

#define STDOUT_FILENO 1

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

   uint64_t now = ((uint64_t)(clockhigh)<<32) | clocklow;
   return now;
}

uint32_t ClockToMs(uint64_t clk)
{
   return (uint32_t)(clk / ONE_MILLISECOND_IN_TICKS);
}

uint32_t ClockToUs(uint64_t clk)
{
   return (uint32_t)(clk / ONE_MICROSECOND_IN_TICKS);
}

void E32SetTimeCompare(const uint64_t future)
{
   // NOTE: ALWAYS set high word first to avoid misfires outside timer interrupt
   swap_csr(0x801, ((future&0xFFFFFFFF00000000)>>32));
   swap_csr(0x800, ((uint32_t)(future&0x00000000FFFFFFFF)));
}

// C stdlib overrides

// Place the heap into DDR3 memory
//#undef errno
//int nerrno;
static uint8_t *heap_start  = (uint8_t*)0x08000000; // Program/static data can leak all the way up to 128MBytes
static uint8_t *heap_end    = (uint8_t*)0x0FFF0000; // ~127MBytes of heap, 64KBytes at the end reserved for now (future kernel stack)

#ifdef __cplusplus
extern "C" {
#endif

   /*int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
   {
     // offset the current task's wakeup time, do not busywait
     // however for our architecture, scheduler will break and run
     // other tasks regardless of what's going on, so we can busy spin here
      errno = ENOSYS;
      return -1;
   }*/

   int _chdir(const char *path)
   {
      /*if (FR_OK == f_chdir(path))
         return 0;
      else*/
      {
         errno = ENOSYS;
         return -1;
      }
   }

   char *_getcwd(char *buf, size_t size)
   {
      /*if (FR_OK == f_getcwd(buf, size))
         return buf;
      else*/
      {
         errno = -ENOSYS;
         return NULL;
      }
   }

   int _isatty(int file)
   {
      return (file == STDOUT_FILENO);
   }

   int _close(int file)
   {
      return 0;
   }

   off_t _lseek(int file, off_t ptr, int dir)
   {
      return 0;
   }

   int _lstat(const char *file, struct stat *st)
   {
      errno = ENOSYS;
      return -1;
   }

   int _open(const char *name, int flags, int mode)
   {
      return -1;
   }

   int _openat(int dirfd, const char *name, int flags, int mode)
   {
      // https://linux.die.net/man/2/openat
      errno = ENOSYS;
      return -1;
   }

   ssize_t _read(int file, void *ptr, size_t len)
   {
      return 0;
   }

   int _stat(const char *file, struct stat *st)
   {
      //st->st_mode = S_IFCHR; // S_IFBLK for disk data?
      return 0;
   }

   ssize_t _write(int file, const void *ptr, size_t len)
   {
      if (file == STDOUT_FILENO) {
         char *cptr = (char*)ptr;
         const char *eptr = cptr + len;
         int i = 0;
         while (cptr != eptr)
         {
            UARTPutChar(*cptr);
            if (i%128==0) // time to flush the FIFO
               UARTFlush();
            ++i;
            ++cptr;
         }
         UARTFlush();
         return len;
      }
      else return 0;
   }

   int _wait(int *status)
   {
      errno = ECHILD;
      return -1;
   }

   void unimplemented_syscall()
   {
   }

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
#ifdef __cplusplus
}
#endif
