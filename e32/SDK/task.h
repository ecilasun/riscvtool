#include <inttypes.h>

extern volatile uint32_t *taskcount;
extern volatile uint32_t *currenttask;
extern volatile struct STaskContext *tasks;

typedef void(*taskfunc)(uint32_t _flags, void * _userdata);

struct STaskContext {
    taskfunc task; // Task entry point
    uint32_t PC; // Program counter
    uint32_t interval; // Execution interval
    uint32_t regs[64]; // Integer and float registers
};

void AddTask(taskfunc _task, uint32_t _stackpointer, uint32_t _interval);
void InitTasks();

#define MAX_TASKS 16

// Save a task's registers (all except TP and GP)
#define SAVEREGS(_tsk) {\
    asm volatile("lw tp, 144(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[1]) ); \
    asm volatile("lw tp, 140(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[2]) ); \
    asm volatile("lw tp, 136(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[5]) ); \
    asm volatile("lw tp, 132(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[6]) ); \
    asm volatile("lw tp, 128(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[7]) ); \
    asm volatile("lw tp, 124(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[10]) ); \
    asm volatile("lw tp, 120(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[11]) ); \
    asm volatile("lw tp, 116(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[12]) ); \
    asm volatile("lw tp, 112(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[13]) ); \
    asm volatile("lw tp, 108(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[14]) ); \
    asm volatile("lw tp, 104(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[15]) ); \
    asm volatile("lw tp, 100(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[16]) ); \
    asm volatile("lw tp,  96(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[17]) ); \
    asm volatile("lw tp,  92(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[28]) ); \
    asm volatile("lw tp,  88(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[29]) ); \
    asm volatile("lw tp,  84(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[30]) ); \
    asm volatile("lw tp,  80(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[31]) ); \
    asm volatile("lw tp,  72(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[8]) ); \
    asm volatile("lw tp,  68(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[9]) ); \
    asm volatile("lw tp,  64(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[18]) ); \
    asm volatile("lw tp,  60(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[19]) ); \
    asm volatile("lw tp,  56(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[20]) ); \
    asm volatile("lw tp,  52(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[21]) ); \
    asm volatile("lw tp,  48(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[22]) ); \
    asm volatile("lw tp,  44(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[23]) ); \
    asm volatile("lw tp,  40(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[24]) ); \
    asm volatile("lw tp,  36(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[25]) ); \
    asm volatile("lw tp,  32(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[26]) ); \
    asm volatile("lw tp,  28(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[27]) ); \
    asm volatile("csrr tp, mepc;  sw tp, %0;" : "=m" (tasks[_tsk].PC) ); }

// Restore a task's registers (all except TP and GP)
#define RESTOREREGS(_tsk) {\
    asm volatile("lw tp, %0; sw tp, 144(sp);" : : "m" (tasks[_tsk].regs[1]) ); \
    asm volatile("lw tp, %0; sw tp, 140(sp);" : : "m" (tasks[_tsk].regs[2]) ); \
    asm volatile("lw tp, %0; sw tp, 136(sp);" : : "m" (tasks[_tsk].regs[5]) ); \
    asm volatile("lw tp, %0; sw tp, 132(sp);" : : "m" (tasks[_tsk].regs[6]) ); \
    asm volatile("lw tp, %0; sw tp, 128(sp);" : : "m" (tasks[_tsk].regs[7]) ); \
    asm volatile("lw tp, %0; sw tp, 124(sp);" : : "m" (tasks[_tsk].regs[10]) ); \
    asm volatile("lw tp, %0; sw tp, 120(sp);" : : "m" (tasks[_tsk].regs[11]) ); \
    asm volatile("lw tp, %0; sw tp, 116(sp);" : : "m" (tasks[_tsk].regs[12]) ); \
    asm volatile("lw tp, %0; sw tp, 112(sp);" : : "m" (tasks[_tsk].regs[13]) ); \
    asm volatile("lw tp, %0; sw tp, 108(sp);" : : "m" (tasks[_tsk].regs[14]) ); \
    asm volatile("lw tp, %0; sw tp, 104(sp);" : : "m" (tasks[_tsk].regs[15]) ); \
    asm volatile("lw tp, %0; sw tp, 100(sp);" : : "m" (tasks[_tsk].regs[16]) ); \
    asm volatile("lw tp, %0; sw tp,  96(sp);" : : "m" (tasks[_tsk].regs[17]) ); \
    asm volatile("lw tp, %0; sw tp,  92(sp);" : : "m" (tasks[_tsk].regs[28]) ); \
    asm volatile("lw tp, %0; sw tp,  88(sp);" : : "m" (tasks[_tsk].regs[29]) ); \
    asm volatile("lw tp, %0; sw tp,  84(sp);" : : "m" (tasks[_tsk].regs[30]) ); \
    asm volatile("lw tp, %0; sw tp,  80(sp);" : : "m" (tasks[_tsk].regs[31]) ); \
    asm volatile("lw tp, %0; sw tp,  72(sp);" : : "m" (tasks[_tsk].regs[8]) ); \
    asm volatile("lw tp, %0; sw tp,  68(sp);" : : "m" (tasks[_tsk].regs[9]) ); \
    asm volatile("lw tp, %0; sw tp,  64(sp);" : : "m" (tasks[_tsk].regs[18]) ); \
    asm volatile("lw tp, %0; sw tp,  60(sp);" : : "m" (tasks[_tsk].regs[19]) ); \
    asm volatile("lw tp, %0; sw tp,  56(sp);" : : "m" (tasks[_tsk].regs[20]) ); \
    asm volatile("lw tp, %0; sw tp,  52(sp);" : : "m" (tasks[_tsk].regs[21]) ); \
    asm volatile("lw tp, %0; sw tp,  48(sp);" : : "m" (tasks[_tsk].regs[22]) ); \
    asm volatile("lw tp, %0; sw tp,  44(sp);" : : "m" (tasks[_tsk].regs[23]) ); \
    asm volatile("lw tp, %0; sw tp,  40(sp);" : : "m" (tasks[_tsk].regs[24]) ); \
    asm volatile("lw tp, %0; sw tp,  36(sp);" : : "m" (tasks[_tsk].regs[25]) ); \
    asm volatile("lw tp, %0; sw tp,  32(sp);" : : "m" (tasks[_tsk].regs[26]) ); \
    asm volatile("lw tp, %0; sw tp,  28(sp);" : : "m" (tasks[_tsk].regs[27]) ); \
    asm volatile("lw tp, %0; csrrw zero, mepc, tp;" : : "m" (tasks[_tsk].PC) ); }

//asm volatile("lw tp, 76(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[3]) );
//asm volatile("lw tp, %0; sw tp, 76(sp);" : : "m" (tasks[_tsk].reg[3]) );
