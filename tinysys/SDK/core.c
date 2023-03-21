// Embedded utils

#include "core.h"
#include "basesystem.h"
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define STDOUT_FILENO 1

#if defined(BUILDING_ROM)

// TODO: __heap_start should be set to __bss_end and __heap_end to __heap_start+__heap_size for loaded executables
// This also means we have to get rid of any addresses between bss and heap as they might get overwritten
static uint8_t* __heap_start = (uint8_t*)HEAP_START_APPMEM_END;
static uint8_t* __heap_end = (uint8_t*)HEAP_END_TASKMEM_START;
static uint8_t* __breakpos = __heap_start;

uint32_t core_memavail()
{
	return (uint32_t)(__heap_end - __breakpos);
}

uint32_t core_brk(uint32_t brkptr)
{
	// Address set to zero will query current break position
	if (brkptr == 0)
		return (uint32_t)__breakpos;

	// Out of bounds will return all ones (-1)
	if (brkptr<(uint32_t)__heap_start || brkptr>(uint32_t)__heap_end)
		return 0xFFFFFFFF;

	// Set new break position and return 0 (success) in all other cases
	__breakpos = (uint8_t*)brkptr;
	return 0;
}

#else

#endif

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

	int _fstat(int fd, struct stat *statbuf)
	{
		// Not implemented
		errno = ENOSYS;
		return -1;
	}

	int _isatty(int file)
	{
		return (file == STDOUT_FILENO);
	}

	int _close(int file)
	{
		register uint32_t cfile = (uint32_t)file;
		register int retval = 0;
		asm (
			"li a7, 57;"
			"mv a0, %1;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (cfile) :
			// Clobber list
			"a0", "a7"
		);
		return retval;
	}

	off_t _lseek(int file, off_t ptr, int dir)
	{
		register uint32_t sfile = (uint32_t)file;
		register uint32_t sptr = (uint32_t)ptr;
		register uint32_t sdir = (uint32_t)dir;
		register int retval = 0;
		asm (
			"li a7, 62;"
			"mv a0, %1;"
			"mv a1, %2;"
			"mv a2, %3;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (sfile), "r" (sptr), "r" (sdir) :
			// Clobber list
			"a0", "a1", "a2", "a7"
		);
		return retval;
	}

	int _lstat(const char *file, struct stat *st)
	{
		/*FILINFO finf;
		if (FR_OK == f_stat(file, &finf))
		{
			st->st_dev = 1;
			st->st_ino = 0;
			st->st_mode = 0; // File mode
			st->st_nlink = 0;
			st->st_uid = 0;
			st->st_gid = 0;
			st->st_rdev = 1;
			st->st_size = finf.fsize;
			st->st_blksize = 512;
			st->st_blocks = (finf.fsize+511)/512;
			st->st_atime = finf.ftime;
			st->st_atime_nsec = 0;
			st->st_mtime = finf.ftime;
			st->st_mtime_nsec = 0;
			st->st_ctime = finf.ftime;
			st->st_ctime_nsec = 0;

			return 0;
		}
		else*/
		{
			errno = ENOSYS;
			return -1;
		}
	}

	int _open(const char *name, int flags, int mode)
	{
		register uint32_t nptr = (uint32_t)name;
		register uint32_t oflags = (uint32_t)flags;
		register uint32_t fmode = (uint32_t)mode;
		register int retval = 0;
		asm (
			"li a7, 1024;"
			"mv a0, %1;"
			"mv a1, %2;"
			"mv a2, %3;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (nptr), "r" (oflags), "r" (fmode) :
			// Clobber list
			"a0", "a1", "a2", "a7"
		);
		return retval;
	}

	int _openat(int dirfd, const char *name, int flags, int mode)
	{
		// https://linux.die.net/man/2/openat
		errno = ENOSYS;
		return -1;
	}

	ssize_t _read(int file, void *ptr, size_t len)
	{
		register uint32_t fptr = (uint32_t)file;
		register uint32_t rptr = (uint32_t)ptr;
		register uint32_t rlen = (uint32_t)len;
		register int retval = 0;
		asm (
			"li a7, 63;"
			"mv a0, %1;"
			"mv a1, %2;"
			"mv a2, %3;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (fptr), "r" (rptr), "r" (rlen) :
			// Clobber list
			"a0", "a1", "a2", "a7"
		);
		return retval;
	}

	int _stat(const char *file, struct stat *st)
	{
		//st->st_mode = S_IFCHR; // S_IFBLK for disk data?
		return 0;
	}

	ssize_t _write(int file, const void *ptr, size_t len)
	{
		register uint32_t fptr = (uint32_t)file;
		register uint32_t wptr = (uint32_t)ptr;
		register uint32_t wlen = (uint32_t)len;
		register int retval = 0;
		asm (
			"li a7, 64;"
			"mv a0, %1;"
			"mv a1, %2;"
			"mv a2, %3;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (fptr), "r" (wptr), "r" (wlen) :
			// Clobber list
			"a0", "a1", "a2", "a7"
		);
		return retval;
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
		register uint32_t brkaddr = (uint32_t)addr;
		register int retval = 0;
		asm (
			"li a7, 214;"
			"mv a0, %1;"
			"ecall;"
			"mv %0, a0;" :
			// Return values
			"=r" (retval) :
			// Input parameters
			"r" (brkaddr) :
			// Clobber list
			"a0", "a7"
		);
		return retval;
	}

	void *_sbrk(intptr_t incr)
	{
		uint8_t *old_heapstart = (uint8_t *)_brk(0);
		uint32_t res = _brk(old_heapstart + incr);
		return res != 0xFFFFFFFF ? old_heapstart : NULL;
	}

#ifdef __cplusplus
}
#endif
