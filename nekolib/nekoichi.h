#pragma once

#include <inttypes.h>
#include <sys/types.h>

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

#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */

#define S_IFDIR	__S_IFDIR
#define S_IFCHR	__S_IFCHR
#define S_IFBLK	__S_IFBLK
#define S_IFREG	__S_IFREG
#define S_IFIFO	__S_IFIFO
#define S_IFLNK	__S_IFLNK
#define S_IFSOCK __S_IFSOCK

// Hardware devices
extern volatile uint32_t *IO_GPUCommandFIFO;
extern volatile uint8_t *IO_UARTRX;
extern volatile uint8_t *IO_UARTTX;
extern volatile uint32_t *IO_UARTRXByteCount;
extern volatile uint8_t *IO_SPIInput;
extern volatile uint8_t *IO_SPIOutput;
extern volatile uint8_t *IO_SwitchState;
extern volatile uint32_t *IO_SwitchByteCount;
extern volatile uint32_t *IO_AudioFIFO;
extern volatile uint32_t *IO_APUCommandFIFO;

// Device memory ranges
extern volatile uint32_t *DDR3Start;
extern volatile uint32_t *GraphicsRAMStart;
extern volatile uint32_t *GraphicsRAMEnd;
extern volatile uint32_t *AudioRAMStart;

void InitFont();
void PrintDMA(const int ox, const int oy, const char *message, bool masked=true);
void PrintDMA(const int col, const int row, const int maxlen, const char *message, bool masked=true);
void PrintDMAHex(const int ox, const int oy, const uint32_t i);
uint32_t PrintDMADecimal(const int ox, const int oy, const int i, bool masked=true);
void EchoUART(const char *_message);
void EchoHex(const uint32_t i);
void ClearScreen(const uint8_t color);
uint32_t Random();
uint64_t ReadClock();
uint64_t ReadRetiredInstructions();
uint32_t ClockToMs(uint64_t clock);
void ClockMsToHMS(uint32_t ms, uint32_t &hours, uint32_t &minutes, uint32_t &seconds);
