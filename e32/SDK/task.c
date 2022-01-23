#include "task.h"

volatile uint32_t *taskcount = (volatile uint32_t *)0x80001F00;
volatile struct STaskContext *tasks = (volatile struct STaskContext *)0x80001F08;

void AddTask(taskfunc _task)
{
    uint32_t prevcount = (*taskcount);

    // TODO: insert the task first

    // Finally, increment the task count
    // TODO: This needs to stop timer interrupts
    // and then resume them to avoid wrong count.
    (*taskcount) = prevcount+1;
}
