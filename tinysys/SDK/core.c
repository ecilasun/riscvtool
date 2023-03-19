// Embedded utils

#include <stdlib.h>
#include <errno.h>
#include "core.h"
#include "uart.h"

#include <sys/stat.h>

#ifndef DISABLE_FILESYSTEM
#include "fat32/ff.h"
#endif

#define STDOUT_FILENO 1

#if defined(BUILDING_ROM)

// TODO: __heap_start should be set to __bss_end and __heap_end to __heap_start+__heap_size for loaded executables
// This also means we have to get rid of any addresses between bss and heap as they might get overwritten
static uint8_t* __heap_start = (uint8_t*)0x02000000;
static uint8_t* __heap_end = (uint8_t*)0x0FFE0000;
static uint8_t* __breakpos = __heap_start;

uint32_t core_memavail()
{
	return (uint32_t)(__heap_end - __heap_start);
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

#ifndef DISABLE_FILESYSTEM
	FIL s_filehandles[32];
	int s_numhandles = 0;
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
	#ifndef DISABLE_FILESYSTEM
		f_close(&s_filehandles[file]);
		return 0;
	#else
		return 0;
	#endif
	}

	off_t _lseek(int file, off_t ptr, int dir)
	{
	#ifndef DISABLE_FILESYSTEM
		FSIZE_t currptr = s_filehandles[file].fptr;
		if (dir == 2 ) // SEEK_END
		{
			DWORD flen = s_filehandles[file].obj.objsize;
			currptr = flen;
		}
		else if (dir == 1) // SEEK_CUR
		{
			currptr = ptr + currptr;
		}
		else// if (dir == 0) // SEEK_SET
		{
			currptr = ptr;
		}

		f_lseek(&s_filehandles[file], currptr);
		return currptr;
	#else
		return 0;
	#endif
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
	#ifndef DISABLE_FILESYSTEM
		int currenthandle = s_numhandles;
		if (FR_OK == f_open(&s_filehandles[currenthandle], name, FA_READ)) //mode))
		{
			++s_numhandles;
			return currenthandle;
		}
		else
		{
			errno = ENOENT;
			return -1;
		}
	#else
		return -1;
	#endif
	}

	int _openat(int dirfd, const char *name, int flags, int mode)
	{
		// https://linux.die.net/man/2/openat
		errno = ENOSYS;
		return -1;
	}

	ssize_t _read(int file, void *ptr, size_t len)
	{
	#ifndef DISABLE_FILESYSTEM
		UINT readlen;
		/*pf_read(file, ptr, len, &readlen);*/
		if (FR_OK == f_read(&s_filehandles[file], ptr, len, &readlen))
			return readlen;
		else
	#endif
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
				*IO_UARTTX = *cptr;
				++i;
				++cptr;
			}
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
		uint32_t brkaddr = (uint32_t)addr;
		int retval = 0;
		asm volatile(
			"li a7, 214;"
			"lw a0, %1;"
			"ecall;"
			"sw a0, %0;" :
			// Return value
			"=m" (retval) :
			// Input parameters
			"m" (brkaddr)
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
