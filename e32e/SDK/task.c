#include "core.h"
#include "task.h"

#include <stdlib.h>

const char *dummyname="dummy";

// NOTE: Call with memory allocated for task tracking purposes
// with sufficient space for MAX_TASKS*sizeof(STaskContext) bytes
void InitTasks(struct STaskContext *_ctx)
{
	// Clean out all tasks
	for (uint32_t i=0; i<MAX_TASKS; ++i)
	{
		_ctx[i].HART = 0x0; // Can't run anywhere by default
		_ctx[i].interval = TEN_MILLISECONDS_IN_TICKS;
		_ctx[i].PC = 0x0;
		_ctx[i].regs[2] = 0x0;
		_ctx[i].regs[8] = 0x0;
		_ctx[i].task = (taskfunc)0x0;
		_ctx[i].ctrlc = 0;
		_ctx[i].breakhit = 0;
		_ctx[i].name = dummyname;
		_ctx[i].num_breakpoints = 0;
		for (int j=0;j<TASK_MAX_BREAKPOINTS;++j)
		{
			_ctx[i].breakpoints[j].address = 0x0;
			_ctx[i].breakpoints[j].originalinstruction = 0x0;
		}
	}
}

// NOTE: Can only add a task to this core's context
// HINT: Use MAILBOX and HARTIRQ to send across a task to be added to a remote core
bool AddTask(struct STaskContext *_ctx, volatile uint32_t *taskcount, taskfunc _task, uint32_t _stacksizeword, uint32_t _interval)
{
	uint32_t prevcount = (*taskcount);
	if (prevcount >= MAX_TASKS)
		return false;

	uint32_t *stackbase = (uint32_t*)malloc(_stacksizeword*sizeof(uint32_t));
	uint32_t *stackpointer = stackbase + (_stacksizeword-1);

	// Insert the task before we increment task count
	_ctx[prevcount].PC = (uint32_t)_task;
	_ctx[prevcount].regs[2] = (uint32_t)stackpointer;	// Stack pointer
	_ctx[prevcount].regs[8] = (uint32_t)stackpointer;	// Frame pointer
	_ctx[prevcount].interval = _interval;				// Execution interval
	_ctx[prevcount].taskstack = stackbase;				// Stack base, to be freed when task's done
	_ctx[prevcount].stacksize = _stacksizeword;			// Stack size in WORDs

	// Stop timer interrupts on this core during this operation
	swap_csr(mie, MIP_MSIP | MIP_MEIP);
	// Increment the task count
	(*taskcount) = prevcount+1;
	// Resume timer interrupts on this core
	swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

	return true;
}

/*
  // Work distributor
  int done = 0;
  uint32_t workunit = 0;
  do {
    // Distribute some work for each hart, which also wakes them up
    for (uint32_t i=1; i<numharts; ++i)
    {
      uint32_t currentworkunit = mailbox[i];
      if (currentworkunit == 0xFFFFFFFF)
      {
        uint32_t x, y;
        EMorton2DDecode(workunit, x, y); // Tile from work index in Morton order
        mailbox[i] = x + 16*y; // Assign tile

        // Start over
        if (workunit == 255)
          done = 1;
      }
    }
  } while (!done);
*/
/*
// Worker CPU entry point
void workermain()
{
  // Wait for mailbox to notify us that HART#0 is ready
  uint32_t hartid = read_csr(mhartid);

  // Wait to be woken up
  uint32_t hartbit = (1<<hartid);
  while ((mailbox[numharts]&hartbit) == 0) { }

  uint32_t workunit = 0xFFFFFFFF;
  do{
    // Wait for valid work unit
    do {
      workunit = mailbox[hartid];
    } while(workunit == 0xFFFFFFFF);
    // Do our work
    // tilex = workunit%16 -> videoTileX = 20*tilex
    // tiley = workunit/16 -> videoTileY = 15*tiley
    myTileWork(v0, v1, v2, workunit);
    // Work done
    mailbox[hartid] = 0xFFFFFFFF;
  } while(1);
}
*/

// Helper for Morton order work dispatch
/*void EMorton2DDecode(const uint32_t morton, uint32_t &x, uint32_t &y)
{
  uint32_t res = morton&0x5555555555555555;
  res=(res|(res>>1)) & 0x3333333333333333;
  res=(res|(res>>2)) & 0x0f0f0f0f0f0f0f0f;
  res=(res|(res>>4)) & 0x00ff00ff00ff00ff;
  res=res|(res>>8);
  x = res;

  res = (morton>>1)&0x5555555555555555;
  res=(res|(res>>1)) & 0x3333333333333333;
  res=(res|(res>>2)) & 0x0f0f0f0f0f0f0f0f;
  res=(res|(res>>4)) & 0x00ff00ff00ff00ff;
  res=res|(res>>8);
  y = res;
}
*/