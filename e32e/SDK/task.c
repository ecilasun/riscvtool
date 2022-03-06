#include "core.h"
#include "task.h"

volatile uint32_t *taskcount = (volatile uint32_t *)0x80000F00;
volatile uint32_t *currenttask = (volatile uint32_t *)0x80000F04;
volatile struct STaskContext *tasks = (volatile struct STaskContext *)0x80000F08;

void InitTasks()
{
    *taskcount = 0;
    *currenttask = 0;
}

void AddTask(taskfunc _task, uint32_t _stackpointer, uint32_t _interval)
{
    uint32_t prevcount = (*taskcount);

    // Insert the task first
    tasks[prevcount].PC = (uint32_t)_task;
    tasks[prevcount].regs[2] = _stackpointer; // Stack pointer
    tasks[prevcount].regs[8] = _stackpointer; // Frame pointer
    tasks[prevcount].interval = _interval;    // Execution interval

    // Stop timer interrupts during this
    swap_csr(mie, MIP_MSIP | MIP_MEIP);
    // Increment the task count
    (*taskcount) = prevcount+1;
    // Resume timer interrupts
    swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);
}
