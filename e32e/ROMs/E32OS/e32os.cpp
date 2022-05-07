// E32OS

#include "rvcrt0.h" // NOTE: Only include once

#include "e32os.h"

#include "workercore.h"
#include "maincore.h"

uint32_t *OSMEM = (uint32_t*)0x1FFF0000; // Last 64K of DDR3 memory, 8KBytes of space per HART

// Worker CPU entry point
void workermain()
{
	uint32_t hartid = read_csr(mhartid);

	// Set data as seen by this core to zero
	OSMEM[hartid] = 16-hartid;

	// Set up our ISR
	InstallWorkerCoreISR();

	// Signal awake
	HARTMAILBOX[hartid] = 0xFFFFFFFF;

	while (1)
	{
		// Halt on wakeup
		asm volatile("wfi;");

		// Make sure we'll re-load from RAM for an accurate image of what's written by HART#0
		asm volatile( ".word 0xFC200073;" ); // CDISCARD.D.L1 (invalidate D$)

		// Output the integer that HART#0 wrote
		UARTWriteDecimal(OSMEM[hartid]);
	}
}

int main()
{
	// Set up main ISR
	InstallMainCoreISR();

	// Wait for all HARTs to wake up
	// At this point, they should all have a different view
	// of memory than the one we're going to create here
	uint32_t awake;
	UARTWrite("Waiting for worker cores.\n");
	do{
		awake = 0;
		for (int i=1; i<NUM_HARTS; ++i)
			awake += HARTMAILBOX[i] == 0xFFFFFFFF ? 1:0;
	} while (awake < 7);
	++awake; // Include this core
	UARTWriteDecimal(awake);
	UARTWrite(" cores awake. Starting cache test.\n");

	// Now go ahead and write some data for each slot in OSMEM corresponding to HART indices
	for (uint32_t i=1; i<NUM_HARTS; ++i)
		OSMEM[i] = i;

	// Make sure this data makes it to RAM
	asm volatile( ".word 0xFC000073;" ); // CFLUSH.D.L1 (writeback D$)

	// Trigger interrupts on all HARTs
	// At this point, each HART should invalidate their D$ and read this new data from RAM
	for (uint32_t i=1; i<NUM_HARTS; ++i)
		HARTIRQ[i] = 1;

	// Report what we see as well
	uint32_t hartid = read_csr(mhartid);
	UARTWriteDecimal(OSMEM[hartid]);

	// Main loop, sleeps most of the time until an interrupt occurs
	UARTWrite("\nAll done, idling...\n");
	while(1)
	{
		asm volatile("wfi;");
	}

	return 0;
}
