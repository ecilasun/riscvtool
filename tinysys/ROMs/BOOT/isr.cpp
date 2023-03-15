#include "encoding.h"
#include "core.h"
#include "debugger.h"
#include "uart.h"
#include "isr.h"
#include "ringbuffer.h"

#include <stdlib.h>

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
	uint32_t PC = read_csr(mepc)-4;		// Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000) // Hardware interrupts
	{
		switch (code)
		{
			case IRQ_M_EXT:
			{
				// TODO: We need to read the 'reason' for HW interrupt from a custom register
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
				UARTWrite("\033[0m\r\n\r\n\033[31m\033[40mUnknown hardware interrupt 0x");
				UARTWriteHex(code);
				UARTWrite("\r\n\033[0m\r\n");
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
				gdb_breakpoint(&g_taskctx);
			}
			break;

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
						UARTWrite("unimpl: io_setup()\r\n");
					}
					break;

					case 57: // close()
					{
						//sys_close(unsigned int fd);
						UARTWrite("unimpl: close()\r\n");
					}
					break;

					case 62: // lseek()
					{
						//sys_llseek(unsigned int fd, unsigned long offset_high, unsigned long offset_low, loff_t __user *result, unsigned int whence);
						UARTWrite("unimpl: lseek()\r\n");
					}
					break;

					case 63: // read()
					{
						//sys_read(unsigned int fd, char __user *buf, size_t count);
						UARTWrite("unimpl: read()\r\n");
					}
					break;

					case 64: // write()
					{
						uint32_t file = read_csr(0x00A); // A0
						uint32_t ptr = read_csr(0x00B); // A1
						uint32_t count = read_csr(0x00C); // A2
						if (file == 1) { // STDOUT_FILENO==1
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
							UARTWrite("unimpl: write(");
							UARTWriteHex(file);
							UARTWrite(", ");
							UARTWriteHex(ptr);
							UARTWrite(", ");
							UARTWriteHex(count);
							UARTWrite(");\r\n");
							write_csr(0x00A, 0);
						}
					}
					break;

					case 80: // newfstat()
					{
						//sys_newfstat(unsigned int fd, struct stat __user *statbuf);
						UARTWrite("unimpl: newfstat()\r\n");
					}
					break;

					case 93: // exit()
					{
						// Terminate and remove from list of running tasks
						TaskExitCurrentTask(&g_taskctx);
					}
					break;

					case 129: // kill(pid_t pid, int sig)
					{
						// Signal process to terminate
						uint32_t pid = read_csr(0x00A); // A0
						uint32_t sig = read_csr(0x00B); // A1
						TaskExitTaskWithID(&g_taskctx, pid, sig);
						UARTWrite("Process killed\r\n");
					}
					break;

					case 214: // brk()
					{
						uint32_t addrs = read_csr(0x00A); // A0
						int retval = core_brk(addrs);
						write_csr(0x00A, retval);
					}
					break;

					case 1024: // open()
					{
						//sys_open(const char __user * filename, int flags, umode_t mode);
						UARTWrite("unimpl: open()\r\n");
					}
					break;

					// Unimplemented syscalls drop here
					default:
					{
						UARTWrite("\033[0m\r\n\r\n\033[31m\033[40m");
						UARTWrite("Unimplemented machine ECALL: #");
						UARTWriteHex((uint32_t)value); // Syscall ID
						UARTWrite(" @");
						UARTWriteHex((uint32_t)PC); // PC
						UARTWrite("\033[0m\r\n");
					}
					break;
				}
			}
			break;

			/*case CAUSE_MISALIGNED_FETCH:
			case CAUSE_FETCH_ACCESS:
			case CAUSE_ILLEGAL_INSTRUCTION:
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
				// reverse on: \033[7m blink on: \033[5m
				// Clear attributes, step down a couple lines and
				// set foreground color to red, bg color to black
				UARTWrite("\033[0m\r\n\r\n\033[31m\033[40m");

				UARTWrite("┌───────────────────────────────────────────────────┐\r\n");
				UARTWrite("│ Software Failure. Press reset button to continue. │\r\n");
				UARTWrite("│   Guru Meditation #");
				UARTWriteHex((uint32_t)cause); // Cause
				UARTWrite(".");
				UARTWriteHex((uint32_t)value); // A7 for no reason
				UARTWrite(" @");
				UARTWriteHex((uint32_t)PC); // PC
				UARTWrite("    │\r\n");
				UARTWrite("└───────────────────────────────────────────────────┘\r\n");
				UARTWrite("\033[0m\r\n");

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