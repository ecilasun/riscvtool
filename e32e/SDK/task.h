#include <inttypes.h>

extern volatile uint32_t *taskcount;
extern volatile uint32_t *currenttask;
extern volatile struct STaskContext *tasks;

typedef void(*taskfunc)(uint32_t _flags, void * _userdata);

struct STaskContext {
    taskfunc task;      // Task entry point
    uint32_t HART;      // HART affinity mask
    uint32_t PC;        // Program counter
    uint32_t interval;  // Execution interval
    uint32_t regs[64];  // Integer and float registers
};

void InitTasks(struct STaskContext *_ctx);
bool AddTask(struct STaskContext *_ctx, taskfunc _task, uint32_t _stackpointer, uint32_t _interval);

#define MAX_TASKS 16


//asm volatile("lw tp, 76(sp); sw tp, %0;" : "=m" (tasks[_tsk].regs[3]) );
//asm volatile("lw tp, %0; sw tp, 76(sp);" : : "m" (tasks[_tsk].reg[3]) );
