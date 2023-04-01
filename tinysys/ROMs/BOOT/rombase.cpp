#include "rombase.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int havedrive = 0;
FATFS *Fs = nullptr;

void ReportError(const uint32_t _width, const char *_error, uint32_t _cause, uint32_t _value, uint32_t _PC)
{
	UARTWrite("\033[0m\r\n\r\n\033[31m\033[40m");

	UARTWrite("┌");
	for (uint32_t w=0;w<_width-2;++w)
		UARTWrite("─");
	UARTWrite("┐\r\n│");

	// Message
	int W = strlen(_error);
	W = (_width-2) - W;
	UARTWrite(_error);
	for (int w=0;w<W;++w)
		UARTWrite(" ");
	UARTWrite("│\r\n|");

	// Cause
	UARTWrite("cause:");
	UARTWriteHex(_cause);
	for (uint32_t w=0;w<_width-16;++w)
		UARTWrite(" ");

	// Value
	UARTWrite("│\r\n|value:");
	UARTWriteHex(_value);
	for (uint32_t w=0;w<_width-16;++w)
		UARTWrite(" ");

	// PC
	UARTWrite("│\r\n|PC:");
	UARTWriteHex(_PC);
	for (uint32_t w=0;w<_width-13;++w)
		UARTWrite(" ");

	UARTWrite("│\r\n└");
	for (uint32_t w=0;w<_width-2;++w)
		UARTWrite("─");
	UARTWrite("┘\r\n\033[0m\r\n");
}

void MountDrive()
{
	if (havedrive)
		return;

	// Attempt to mount file system on micro-SD card
	Fs = (FATFS*)malloc(sizeof(FATFS));
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
	{
		havedrive = 0;
		ReportError(32, "File system error", mountattempt, 0, 0);
	}
	else
	{
		havedrive = 1;
	}
}

void UnmountDrive()
{
	if (havedrive)
	{
		f_mount(nullptr, "sd:", 1);
		free(Fs);
		havedrive = 0;
	}
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
			if (re != FR_OK || finf.fname[0] == 0) // Done scanning dir, or error encountered
				break;

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
			UARTWrite("\033[0m\r\n");
		} while(1);

		f_closedir(&dir);
	}
	else
	{
		UARTWrite("fs error: 0x");
		UARTWriteHex(re);
		UARTWrite("\r\n");
	}
}

uint32_t ParseELFHeaderAndLoadSections(FIL *fp, SElfFileHeader32 *fheader, uint32_t &jumptarget)
{
	uint32_t heap_start = 0;
	if (fheader->m_Magic != 0x464C457F)
	{
		ReportError(32, "ELF header error", fheader->m_Magic, 0, 0);
		return heap_start;
	}

	jumptarget = fheader->m_Entry;
	UINT bytesread = 0;

	// Read program headers
	for (uint32_t i=0; i<fheader->m_PHNum; ++i)
	{
		SElfProgramHeader32 pheader;
		f_lseek(fp, fheader->m_PHOff + fheader->m_PHEntSize*i);
		f_read(fp, &pheader, sizeof(SElfProgramHeader32), &bytesread);

		// Something here
		if (pheader.m_MemSz != 0)
		{
			uint8_t *memaddr = (uint8_t *)pheader.m_PAddr;
			// Initialize the memory range at target physical address
			// This can be larger than the loaded size
			memset(memaddr, 0x0, pheader.m_MemSz);
			// Load the binary
			f_lseek(fp, pheader.m_Offset);
			f_read(fp, memaddr, pheader.m_FileSz, &bytesread);
			uint32_t blockEnd = (uint32_t)memaddr + pheader.m_MemSz;
			heap_start = heap_start < blockEnd ? blockEnd : heap_start;
		}
	}

	return E32AlignUp(heap_start, 1024);
}

uint32_t LoadExecutable(const char *filename, const bool reportError)
{
	FIL fp;
	FRESULT fr = f_open(&fp, filename, FA_READ);
	if (fr == FR_OK)
	{
		SElfFileHeader32 fheader;
		UINT readsize;
		f_read(&fp, &fheader, sizeof(fheader), &readsize);
		uint32_t branchaddress;
		uint32_t heap_start = ParseELFHeaderAndLoadSections(&fp, &fheader, branchaddress);
		f_close(&fp);

		asm volatile(
			".word 0xFC000073;" // Invalidate & Write Back D$ (CFLUSH.D.L1)
			"fence.i;"          // Invalidate I$
		);

		// Set brk() to end of BSS
		set_elf_heap(heap_start);

		return branchaddress;
	}
	else
	{
		if (reportError)
			ReportError(32, "Executable not found", fr, 0, 0);
	}

	return 0;
}

#define MAX_HANDLES 32

// Handle allocation mask, positions 0,1 and 2 are reserved
//0	Standard input	STDIN_FILENO	stdin
//1	Standard output	STDOUT_FILENO	stdout
//2	Standard error	STDERR_FILENO	stderr
static uint32_t s_handleAllocMask = 0x00000007;

static FIL s_filehandles[MAX_HANDLES];
static char s_fileNames[MAX_HANDLES][64];

static STaskContext g_taskctx;

STaskContext *CreateTaskContext()
{
	// Initialize task context memory
	TaskInitSystem(&g_taskctx);

	return &g_taskctx;
}

STaskContext *GetTaskContext()
{
	return &g_taskctx;
}

void HandleUART()
{
	// XOFF - hold data traffic from sender so that we don't get stuck here
	// *IO_UARTTX = 0x13;

	// Echo back incoming bytes
	while (UARTInputFifoHasData())
	{
		// Store incoming character in ring buffer
		uint32_t incoming = *IO_UARTRX;
		RingBufferWrite(&incoming, sizeof(uint32_t));
	}

	// XON - resume data traffic from sender
	// *IO_UARTTX = 0x11;
}

void TaskDebugMode(uint32_t _mode)
{
	g_taskctx.debugMode = _mode;
}

uint32_t FindFreeFileHandle(const uint32_t _input)
{
	uint32_t tmp = _input;
	for (uint32_t i=0; i<32; ++i)
	{
		if ((tmp & 0x00000001) == 0)
			return (i+1);
		tmp = tmp >> 1;
	}

	return 0;
}

void AllocateFileHandle(const uint32_t _bitIndex, uint32_t * _input)
{
	uint32_t mask = 1 << (_bitIndex-1);
	*_input = (*_input) | mask;
}

void ReleaseFileHandle(const uint32_t _bitIndex, uint32_t * _input)
{
	uint32_t mask = 1 << (_bitIndex-1);
	*_input = (*_input) & (~mask);
}

uint32_t IsFileHandleAllocated(const uint32_t _bitIndex, const uint32_t  _input)
{
	uint32_t mask = 1 << (_bitIndex-1);
	return (_input & mask) ? 1 : 0;
}

//void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) interrupt_service_routine() // Auto-saves registers
void __attribute__((aligned(16))) __attribute__((naked)) interrupt_service_routine() // Manual register save
{
	// Use extra space in CSR file to store a copy of the current register set before we overwrite anything
	// TODO: Used vectored interrupts so we don't have to do this for all kinds of interrupts
	asm volatile(" \
		csrw 0x001, ra; \
		csrw 0x002, sp; \
		csrw 0x003, gp; \
		csrw 0x004, tp; \
		csrw 0x005, t0; \
		csrw 0x006, t1; \
		csrw 0x007, t2; \
		csrw 0x008, s0; \
		csrw 0x009, s1; \
		csrw 0x00A, a0; \
		csrw 0x00B, a1; \
		csrw 0x00C, a2; \
		csrw 0x00D, a3; \
		csrw 0x00E, a4; \
		csrw 0x00F, a5; \
		csrw 0x010, a6; \
		csrw 0x011, a7; \
		csrw 0x012, s2; \
		csrw 0x013, s3; \
		csrw 0x014, s4; \
		csrw 0x015, s5; \
		csrw 0x016, s6; \
		csrw 0x017, s7; \
		csrw 0x018, s8; \
		csrw 0x019, s9; \
		csrw 0x01A, s10; \
		csrw 0x01B, s11; \
		csrw 0x01C, t3; \
		csrw 0x01D, t4; \
		csrw 0x01E, t5; \
		csrw 0x01F, t6; \
		csrr a5, mepc; \
		csrw 0x000, a5; \
	");

	// CSR[0x011] now contains A7 (SYSCALL number)
	uint32_t value = read_csr(0x011);	// Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);	// Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	uint32_t PC = read_csr(mepc);		// Return address == crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000) // Hardware interrupts
	{
		switch (code)
		{
			case IRQ_M_EXT:
			{
				// TODO: We need to read the 'reason' for HW interrupt from mscratch
				// For now, ignore the reason as we only have one possible IRQ generator.
				if (g_taskctx.debugMode)
					gdb_handler(&g_taskctx);
				else
					HandleUART();

				// Machine External Interrupt (hardware)
				// Route based on hardware type
				//if (value & 0x00000001) HandleUART();
				//if (value & 0x00000002) HandleKeyboard();
				//if (value & 0x00000004) HandleJTAG();
			}
			break;

			case IRQ_M_TIMER:
			{
				// Machine Timer Interrupt (timer)
				// Task scheduler runs here

				// Switch between running tasks
				// TODO: Return time slice request of current task
				uint32_t runLength = TaskSwitchToNext(&g_taskctx);

				// Task scheduler will re-visit after we've filled run length of this task
				uint64_t now = E32ReadTime();
				// TODO: Use time slice request returned from TaskSwitchToNext()
				uint64_t future = now + runLength;
				E32SetTimeCompare(future);
			}
			break;

			default:
				ReportError(32, "Unknown hardware interrupt", cause, value, PC);

				// Put core to endless sleep
				while(1) {
					asm volatile("wfi;");
				}
			break;
		}
	}
	else // Exceptions
	{
		switch(code)
		{
			case CAUSE_BREAKPOINT:
			{
				if (g_taskctx.debugMode)
					gdb_breakpoint(&g_taskctx);
				else
					; // Ignore breakpoint instruction in non-debug mode
			}
			break;
		
			case CAUSE_ILLEGAL_INSTRUCTION:
			{
				uint32_t instruction = *((uint32_t*)PC);
				ReportError(32, "Illegal instruction", cause, instruction, PC);

				// Terminate and remove from list of running tasks
				TaskExitCurrentTask(&g_taskctx);

				// TODO: Automatically add a debug breakpoint here and halt (or jump into gdb_breakpoint)
			}

			case CAUSE_MACHINE_ECALL:
			{
				switch(value)
				{
					// See: https://jborza.com/post/2021-05-11-riscv-linux-syscalls/
					// 0	io_setup	long sys_io_setup(unsigned nr_reqs, aio_context_t __user *ctx);
					// 57	close		long sys_close(unsigned int fd);
					// 62	lseek		long sys_llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low, loff_t __user *result, unsigned int whence);
					// 63	read		long sys_read(unsigned int fd, char __user *buf, size_t count);
					// 64	write		long sys_write(unsigned int fd, const char __user *buf, size_t count);
					// 80	newfstat	long sys_newfstat(unsigned int fd, struct stat __user *statbuf);
					// 93	exit		long sys_exit(int error_code);
					// 129	kill		long sys_kill(pid_t pid, int sig);
					// 214	brk			long sys_brk(void* brk);
					// 1024	open		long sys_open(const char __user * filename, int flags, umode_t mode); open/create file

					case 0: // io_setup()
					{
						//sys_io_setup(unsigned nr_reqs, aio_context_t __user *ctx);
						ReportError(32, "unimpl: io_setup()", cause, value, PC);
						write_csr(0x00A, 0xFFFFFFFF);
					}
					break;

					case 57: // close()
					{
						uint32_t file = read_csr(0x00A); // A0

						if (file > STDERR_FILENO) // Won't let stderr, stdout and stdin be closed
						{
							ReleaseFileHandle(file, &s_handleAllocMask);
							f_close(&s_filehandles[file]);
						}
						write_csr(0x00A, 0);
					}
					break;

					case 62: // lseek()
					{
						// NOTE: We do not support 'holes' in files

						uint32_t file = read_csr(0x00A); // A0
						uint32_t offset = read_csr(0x00B); // A1
						uint32_t whence = read_csr(0x00C); // A2

						// Grab current cursor
						FSIZE_t currptr = s_filehandles[file].fptr;

						if (whence == 2 ) // SEEK_END
						{
							// Offset from end of file
							currptr = offset + s_filehandles[file].obj.objsize;
						}
						else if (whence == 1) // SEEK_CUR
						{
							// Offset from current position
							currptr = offset + currptr;
						}
						else// if (whence == 0) // SEEK_SET
						{
							// Direct offset
							currptr = offset;
						}
						f_lseek(&s_filehandles[file], currptr);
						write_csr(0x00A, currptr);
					}
					break;

					case 63: // read()
					{
						UINT readlen = 0;
						uint32_t file = read_csr(0x00A); // A0
						uint32_t ptr = read_csr(0x00B); // A1
						uint32_t len = read_csr(0x00C); // A2

						if (file == STDIN_FILENO)
						{
							// TODO: Maybe read one characer from UART here?
							errno = EIO;
							write_csr(0x00A, 0xFFFFFFFF);
						}
						else if (file == STDOUT_FILENO)
						{
							// Can't read from stdout
							errno = EIO;
							write_csr(0x00A, 0xFFFFFFFF);
						}
						else if (file == STDERR_FILENO)
						{
							// Can't read from stderr
							errno = EIO;
							write_csr(0x00A, 0xFFFFFFFF);
						}
						else // Any other ordinary file
						{
							if (IsFileHandleAllocated(file, s_handleAllocMask) && (FR_OK == f_read(&s_filehandles[file], (void*)ptr, len, &readlen)))
							{
								write_csr(0x00A, readlen);
							}
							else
							{
								errno = EIO;
								write_csr(0x00A, 0xFFFFFFFF);
							}
						}
					}
					break;

					case 64: // write()
					{
						uint32_t file = read_csr(0x00A); // A0
						uint32_t ptr = read_csr(0x00B); // A1
						uint32_t count = read_csr(0x00C); // A2

						if (file == STDOUT_FILENO || file == STDERR_FILENO)
						{
							char *cptr = (char*)ptr;
							const char *eptr = cptr + count;
							int i = 0;
							while (cptr != eptr)
							{
								*IO_UARTTX = *cptr;
								++i;
								++cptr;
							}
							write_csr(0x00A, i);
						}
						else
						{
							UINT writtenbytes = 0;
							if (FR_OK == f_write(&s_filehandles[file], (const void*)ptr, count, &writtenbytes))
								write_csr(0x00A, writtenbytes);
							else
							{
								errno = EACCES;
								write_csr(0x00A, 0xFFFFFFFF);
							}
						}
					}
					break;

					case 80: // newfstat()
					{
						uint32_t fd = read_csr(0x00A); // A0
						uint32_t ptr = read_csr(0x00B); // A1
						struct stat *buf = (struct stat *)ptr;

						if (fd < 0)
						{
							errno = EBADF;
							write_csr(0x00A, 0xFFFFFFFF);
						}
						else
						{
							if (fd < STDERR_FILENO)
							{
								buf->st_dev = 1;
								buf->st_ino = 0;
								buf->st_mode = S_IFCHR; // character device
								buf->st_nlink = 0;
								buf->st_uid = 0;
								buf->st_gid = 0;
								buf->st_rdev = 1;
								buf->st_size = 0;
								buf->st_blksize = 0;
								buf->st_blocks = 0;
								buf->st_atim.tv_sec = 0;
								buf->st_atim.tv_nsec = 0;
								buf->st_mtim.tv_sec = 0;
								buf->st_mtim.tv_nsec = 0;
								buf->st_ctim.tv_sec = 0;
								buf->st_ctim.tv_nsec = 0;
								write_csr(0x00A, 0x0);
							}
							else
							{
								FILINFO finf;
								FRESULT fr = f_stat(s_fileNames[fd], &finf);

								if (fr != FR_OK)
								{
									errno = ENOENT;
									write_csr(0x00A, 0xFFFFFFFF);
								}
								else
								{
									buf->st_dev = 0;
									buf->st_ino = 0;
									buf->st_mode = S_IFREG; // regular file
									buf->st_nlink = 0;
									buf->st_uid = 0;
									buf->st_gid = 0;
									buf->st_rdev = 0;
									buf->st_size = finf.fsize;
									buf->st_blksize = 512;
									buf->st_blocks = (finf.fsize+511)/512;
									buf->st_atim.tv_sec = finf.ftime;
									buf->st_atim.tv_nsec = 0;
									buf->st_mtim.tv_sec = 0;
									buf->st_mtim.tv_nsec = 0;
									buf->st_ctim.tv_sec = 0;
									buf->st_ctim.tv_nsec = 0;
									write_csr(0x00A, 0x0);
								}
							}
						}
					}
					break;

					case 93: // exit()
					{
						// Terminate and remove from list of running tasks
						TaskExitCurrentTask(&g_taskctx);
						write_csr(0x00A, 0x0);
					}
					break;

					case 95: // wait()
					{
						// Wait for child process status change - unused
						// pid_t wait(int *wstatus);
						ReportError(32, "unimpl: wait()", cause, value, PC);
						errno = ECHILD;
						write_csr(0x00A, 0xFFFFFFFF);
					}
					break;

					case 129: // kill(pid_t pid, int sig)
					{
						// Signal process to terminate
						uint32_t pid = read_csr(0x00A); // A0
						uint32_t sig = read_csr(0x00B); // A1
						TaskExitTaskWithID(&g_taskctx, pid, sig);
						write_csr(0x00A, 0x0);
						UARTWrite("Task killed\r\n");
					}
					break;

					case 214: // brk()
					{
						uint32_t addrs = read_csr(0x00A); // A0
						uint32_t retval = core_brk(addrs);

						write_csr(0x00A, retval);
					}
					break;

					case 1024: // open()
					{
						uint32_t nptr = read_csr(0x00A); // A0
						uint32_t oflags = read_csr(0x00B); // A1
						//uint32_t pmode = read_csr(0x00C); // A2 - permission mode unused for now

						BYTE ff_flags = FA_READ;
						switch (oflags & 3)
						{
							case 00: ff_flags = FA_READ; break; // O_RDONLY
							case 01: ff_flags = FA_WRITE; break; // O_WRONLY
							case 02: ff_flags = FA_READ|FA_WRITE; break; // O_RDWR
							default: ff_flags = FA_READ; break;
						}
						ff_flags |= (oflags&100) ? FA_CREATE_ALWAYS : 0; // O_CREAT
						ff_flags |= (oflags&2000) ? FA_OPEN_APPEND : 0; // O_APPEND

						// Grab lowest zero bit's index
						int currenthandle = FindFreeFileHandle(s_handleAllocMask);

						if (currenthandle == 0)
						{
							errno = ENFILE;
							write_csr(0x00A, 0xFFFFFFFF);
						}
						else
						{
							if (FR_OK == f_open(&s_filehandles[currenthandle], (const TCHAR*)nptr, ff_flags))
							{
								AllocateFileHandle(currenthandle, &s_handleAllocMask);
								write_csr(0x00A, currenthandle);
								strncpy(s_fileNames[currenthandle], (const TCHAR*)nptr, 64);
							}
							else
							{
								errno = ENOENT;
								write_csr(0x00A, 0xFFFFFFFF);
							}
						}
					}
					break;

					// Unimplemented syscalls drop here
					default:
					{
						ReportError(32, "unimplemented ECALL", cause, value, PC);
						errno = EIO;
						write_csr(0x00A, 0xFFFFFFFF);
					}
					break;
				}
			}
			break;

			/*case CAUSE_MISALIGNED_FETCH:
			case CAUSE_FETCH_ACCESS:
			case CAUSE_MISALIGNED_LOAD:
			case CAUSE_LOAD_ACCESS:
			case CAUSE_MISALIGNED_STORE:
			case CAUSE_STORE_ACCESS:
			case CAUSE_USER_ECALL:
			case CAUSE_SUPERVISOR_ECALL:
			case CAUSE_HYPERVISOR_ECALL:
			case CAUSE_FETCH_PAGE_FAULT:
			case CAUSE_LOAD_PAGE_FAULT:
			case CAUSE_STORE_PAGE_FAULT:*/
			default:
			{
				ReportError(32, "Guru meditation", cause, value, PC);

				// Put core to endless sleep
				while(1) {
					asm volatile("wfi;");
				}
			}
			break;
		}
	}

	// Restore registers to next task's register set
	// TODO: Used vectored interrupts so we don't have to do this for all kinds of interrupts
	asm volatile(" \
		csrr a5, 0x000; \
		csrw mepc, a5; \
		csrr ra,  0x001; \
		csrr sp,  0x002; \
		csrr gp,  0x003; \
		csrr tp,  0x004; \
		csrr t0,  0x005; \
		csrr t1,  0x006; \
		csrr t2,  0x007; \
		csrr s0,  0x008; \
		csrr s1,  0x009; \
		csrr a0,  0x00A; \
		csrr a1,  0x00B; \
		csrr a2,  0x00C; \
		csrr a3,  0x00D; \
		csrr a4,  0x00E; \
		csrr a5,  0x00F; \
		csrr a6,  0x010; \
		csrr a7,  0x011; \
		csrr s2,  0x012; \
		csrr s3,  0x013; \
		csrr s4,  0x014; \
		csrr s5,  0x015; \
		csrr s6,  0x016; \
		csrr s7,  0x017; \
		csrr s8,  0x018; \
		csrr s9,  0x019; \
		csrr s10, 0x01A; \
		csrr s11, 0x01B; \
		csrr t3,  0x01C; \
		csrr t4,  0x01D; \
		csrr t5,  0x01E; \
		csrr t6,  0x01F; \
		mret; \
	");
}

void InstallISR()
{
	// Set machine trap vector
	write_csr(mtvec, interrupt_service_routine);

	// Set up timer interrupt one second into the future
	uint64_t now = E32ReadTime();
	uint64_t future = now + TWO_HUNDRED_FIFTY_MILLISECONDS_IN_TICKS;
	E32SetTimeCompare(future);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Enable machine timer interrupts
	write_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	// Allow all machine interrupts to trigger (thus also enabling task system)
	write_csr(mstatus, MSTATUS_MIE);
}
