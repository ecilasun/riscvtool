#pragma once

#include <inttypes.h>
#include <sys/types.h>

#include <encoding.h>

struct stat {
	unsigned long	st_dev;		/* Device.  */
	unsigned long	st_ino;		/* File serial number.  */
	unsigned int	st_mode;	/* File mode.  */
	unsigned int	st_nlink;	/* Link count.  */
	unsigned int	st_uid;		/* User ID of the file's owner.  */
	unsigned int	st_gid;		/* Group ID of the file's group. */
	unsigned long	st_rdev;	/* Device number, if device.  */
	unsigned long	__pad1;
	long		st_size;	/* Size of file, in bytes.  */
	int		st_blksize;	/* Optimal block size for I/O.  */
	int		__pad2;
	long		st_blocks;	/* Number 512-byte blocks allocated. */
	long		st_atime;	/* Time of last access.  */
	unsigned long	st_atime_nsec;
	long		st_mtime;	/* Time of last modification.  */
	unsigned long	st_mtime_nsec;
	long		st_ctime;	/* Time of last status change.  */
	unsigned long	st_ctime_nsec;
	unsigned int	__unused4;
	unsigned int	__unused5;
};

#define ONE_SECOND_IN_TICKS 10000000
#define ONE_MILLISECOND_IN_TICKS 10000
#define ONE_MICROSECOND_IN_TICKS 10

uint64_t E32ReadTime();
uint32_t ClockToMs(uint64_t clk);
uint32_t ClockToUs(uint64_t clk);
void E32SetTimeCompare(const uint64_t future);
