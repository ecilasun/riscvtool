#include <inttypes.h>

typedef void(*taskfunc)();

struct STaskBreakpoint
{
	uint32_t address;				// Address of replaced instruction / breakpoint
	uint32_t originalinstruction;	// Instruction we replaced with EBREAK
};

struct STaskContext {
    taskfunc task;      // Task entry point
    uint32_t HART;      // HART affinity mask
    uint32_t PC;        // Program counter
    uint32_t interval;  // Execution interval
    uint32_t regs[64];  // Integer and float registers
	void *taskstack;	// Allocated by task caller
	uint32_t stacksize;	// Size of stack to set up stack pointer with
	uint32_t ctrlc;		// Stop on first chance
	uint32_t breakhit;	// Breakpoint hit
	uint32_t num_breakpoints;				// Breakpoint count
	struct STaskBreakpoint breakpoints[16];	// Breakpoints
	const char *name;						// Task name
};

// Save a task's registers (all except TP and GP)
#define SAVEREGS(_tsk) {\
	asm volatile("lw tp, 144(sp); sw tp, %0;" : "=m" (_tsk.regs[1]) ); \
	asm volatile("lw tp, 140(sp); sw tp, %0;" : "=m" (_tsk.regs[2]) ); \
	asm volatile("lw tp, 136(sp); sw tp, %0;" : "=m" (_tsk.regs[5]) ); \
	asm volatile("lw tp, 132(sp); sw tp, %0;" : "=m" (_tsk.regs[6]) ); \
	asm volatile("lw tp, 128(sp); sw tp, %0;" : "=m" (_tsk.regs[7]) ); \
	asm volatile("lw tp, 124(sp); sw tp, %0;" : "=m" (_tsk.regs[10]) ); \
	asm volatile("lw tp, 120(sp); sw tp, %0;" : "=m" (_tsk.regs[11]) ); \
	asm volatile("lw tp, 116(sp); sw tp, %0;" : "=m" (_tsk.regs[12]) ); \
	asm volatile("lw tp, 112(sp); sw tp, %0;" : "=m" (_tsk.regs[13]) ); \
	asm volatile("lw tp, 108(sp); sw tp, %0;" : "=m" (_tsk.regs[14]) ); \
	asm volatile("lw tp, 104(sp); sw tp, %0;" : "=m" (_tsk.regs[15]) ); \
	asm volatile("lw tp, 100(sp); sw tp, %0;" : "=m" (_tsk.regs[16]) ); \
	asm volatile("lw tp,  96(sp); sw tp, %0;" : "=m" (_tsk.regs[17]) ); \
	asm volatile("lw tp,  92(sp); sw tp, %0;" : "=m" (_tsk.regs[28]) ); \
	asm volatile("lw tp,  88(sp); sw tp, %0;" : "=m" (_tsk.regs[29]) ); \
	asm volatile("lw tp,  84(sp); sw tp, %0;" : "=m" (_tsk.regs[30]) ); \
	asm volatile("lw tp,  80(sp); sw tp, %0;" : "=m" (_tsk.regs[31]) ); \
	asm volatile("lw tp,  72(sp); sw tp, %0;" : "=m" (_tsk.regs[8]) ); \
	asm volatile("lw tp,  68(sp); sw tp, %0;" : "=m" (_tsk.regs[9]) ); \
	asm volatile("lw tp,  64(sp); sw tp, %0;" : "=m" (_tsk.regs[18]) ); \
	asm volatile("lw tp,  60(sp); sw tp, %0;" : "=m" (_tsk.regs[19]) ); \
	asm volatile("lw tp,  56(sp); sw tp, %0;" : "=m" (_tsk.regs[20]) ); \
	asm volatile("lw tp,  52(sp); sw tp, %0;" : "=m" (_tsk.regs[21]) ); \
	asm volatile("lw tp,  48(sp); sw tp, %0;" : "=m" (_tsk.regs[22]) ); \
	asm volatile("lw tp,  44(sp); sw tp, %0;" : "=m" (_tsk.regs[23]) ); \
	asm volatile("lw tp,  40(sp); sw tp, %0;" : "=m" (_tsk.regs[24]) ); \
	asm volatile("lw tp,  36(sp); sw tp, %0;" : "=m" (_tsk.regs[25]) ); \
	asm volatile("lw tp,  32(sp); sw tp, %0;" : "=m" (_tsk.regs[26]) ); \
	asm volatile("lw tp,  28(sp); sw tp, %0;" : "=m" (_tsk.regs[27]) ); \
	asm volatile("csrr tp, mepc;  sw tp, %0;" : "=m" (_tsk.PC) ); }

// Restore a task's registers (all except TP and GP)
#define RESTOREREGS(_tsk) {\
	asm volatile("lw tp, %0; sw tp, 144(sp);" : : "m" (_tsk.regs[1]) ); \
	asm volatile("lw tp, %0; sw tp, 140(sp);" : : "m" (_tsk.regs[2]) ); \
	asm volatile("lw tp, %0; sw tp, 136(sp);" : : "m" (_tsk.regs[5]) ); \
	asm volatile("lw tp, %0; sw tp, 132(sp);" : : "m" (_tsk.regs[6]) ); \
	asm volatile("lw tp, %0; sw tp, 128(sp);" : : "m" (_tsk.regs[7]) ); \
	asm volatile("lw tp, %0; sw tp, 124(sp);" : : "m" (_tsk.regs[10]) ); \
	asm volatile("lw tp, %0; sw tp, 120(sp);" : : "m" (_tsk.regs[11]) ); \
	asm volatile("lw tp, %0; sw tp, 116(sp);" : : "m" (_tsk.regs[12]) ); \
	asm volatile("lw tp, %0; sw tp, 112(sp);" : : "m" (_tsk.regs[13]) ); \
	asm volatile("lw tp, %0; sw tp, 108(sp);" : : "m" (_tsk.regs[14]) ); \
	asm volatile("lw tp, %0; sw tp, 104(sp);" : : "m" (_tsk.regs[15]) ); \
	asm volatile("lw tp, %0; sw tp, 100(sp);" : : "m" (_tsk.regs[16]) ); \
	asm volatile("lw tp, %0; sw tp,  96(sp);" : : "m" (_tsk.regs[17]) ); \
	asm volatile("lw tp, %0; sw tp,  92(sp);" : : "m" (_tsk.regs[28]) ); \
	asm volatile("lw tp, %0; sw tp,  88(sp);" : : "m" (_tsk.regs[29]) ); \
	asm volatile("lw tp, %0; sw tp,  84(sp);" : : "m" (_tsk.regs[30]) ); \
	asm volatile("lw tp, %0; sw tp,  80(sp);" : : "m" (_tsk.regs[31]) ); \
	asm volatile("lw tp, %0; sw tp,  72(sp);" : : "m" (_tsk.regs[8]) ); \
	asm volatile("lw tp, %0; sw tp,  68(sp);" : : "m" (_tsk.regs[9]) ); \
	asm volatile("lw tp, %0; sw tp,  64(sp);" : : "m" (_tsk.regs[18]) ); \
	asm volatile("lw tp, %0; sw tp,  60(sp);" : : "m" (_tsk.regs[19]) ); \
	asm volatile("lw tp, %0; sw tp,  56(sp);" : : "m" (_tsk.regs[20]) ); \
	asm volatile("lw tp, %0; sw tp,  52(sp);" : : "m" (_tsk.regs[21]) ); \
	asm volatile("lw tp, %0; sw tp,  48(sp);" : : "m" (_tsk.regs[22]) ); \
	asm volatile("lw tp, %0; sw tp,  44(sp);" : : "m" (_tsk.regs[23]) ); \
	asm volatile("lw tp, %0; sw tp,  40(sp);" : : "m" (_tsk.regs[24]) ); \
	asm volatile("lw tp, %0; sw tp,  36(sp);" : : "m" (_tsk.regs[25]) ); \
	asm volatile("lw tp, %0; sw tp,  32(sp);" : : "m" (_tsk.regs[26]) ); \
	asm volatile("lw tp, %0; sw tp,  28(sp);" : : "m" (_tsk.regs[27]) ); \
	asm volatile("lw tp, %0; csrrw zero, mepc, tp;" : : "m" (_tsk.PC) ); }
