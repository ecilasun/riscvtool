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