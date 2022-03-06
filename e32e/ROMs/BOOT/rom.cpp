// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "rvcrt0.h"
#include "encoding.h" // For CSR access macros

#include "core.h"
#include "uart.h"

// CPU synchronization mailbox (uncached access, writes visible to all HARTs the following clock)
volatile uint32_t *mailbox = (volatile uint32_t*)0x20005000;
// Number of HARTs in E32E
static const int numharts = 2;

// Worker CPU entry point
void workermain()
{
  // Wait for mailbox to notify us that HART#0 is ready
  uint32_t hartid = read_csr(mhartid);

  // Since we're reading from mailbox memory (uncached) all writes are visible to all HARTs
  uint32_t hartbit = (1<<hartid);
  while (((*mailbox)&hartbit) == 0) { }

  // HART woke up, output our ID
  UARTWrite("Hello from HART#");
  UARTWriteDecimal(hartid);
  UARTWrite("\n");
  UARTFlush();

  *mailbox = 0x00000000; // Signal 'done' to HART#0

  while(1) { }
}

// Main CPU entry point
int main()
{
  uint32_t hartid = read_csr(mhartid);

  // TODO: HART0 should do all one-time init here before waking up the workers
  UARTWrite("Hello from HART#");
  UARTWriteDecimal(hartid);
  UARTWrite("\n");
  UARTFlush();

  // Enable HART#1 (2), HART#2 (4), HART#3 (8)
  *mailbox = 0x0000000E;

  // TODO: All HARTs are awake now, talk via mailbox to handle sync between them

  // Wait for HART#1 to signal done
  while (*mailbox != 0) { }

  UARTWrite("All done\n");

  while(1) { }

  return 0;
}
