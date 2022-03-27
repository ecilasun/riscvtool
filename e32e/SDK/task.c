#include "core.h"
#include "task.h"

volatile uint32_t *taskcount = (volatile uint32_t *)0x80000FF4;
volatile uint32_t *currenttask = (volatile uint32_t *)0x80000FF8;
//volatile struct STaskContext *taskcontext = (volatile struct STaskContext *)0x80000F08;

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
	}

	*taskcount = 0;
	*currenttask = 0;
}

// NOTE: Can only add a task to this core's context
// HINT: Use MAILBOX and HARTIRQ to send across a task to be added to a remote core
bool AddTask(struct STaskContext *_ctx, taskfunc _task, uint32_t _stackpointer, uint32_t _interval)
{
	uint32_t prevcount = (*taskcount);
	if (prevcount >= MAX_TASKS)
		return false;

	// Insert the task before we increment task count
	_ctx[prevcount].PC = (uint32_t)_task;
	_ctx[prevcount].regs[2] = _stackpointer; // Stack pointer
	_ctx[prevcount].regs[8] = _stackpointer; // Frame pointer
	_ctx[prevcount].interval = _interval;    // Execution interval

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