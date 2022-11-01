// Bootloader

// (c)2022 Engin Cilasun

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdint.h>
#include <math.h>

#include "rvcrt0.h"

#include "core.h"
#include "leds.h"
#include "uart.h"
#include "xadc.h"
#include "gpu.h"
#include "isr.h"

// HART[1..N-1] entry point
void workermain()
{
	//uint32_t hartid = read_csr(mhartid);

	while(1)
	{
		// TODO: Wake up to process hardware interrupt requests
		asm volatile("wfi;");
	}
}

// HART[0] entry point
int main()
{
 	// Reset debug LEDs
	LEDSetState(0x0);

  // Set low bit RGB color palette
  /*uint8_t target = 0;
  for (int b=0;b<4;++b)
    for (int g=0;g<8;++g)
      for (int r=0;r<8;++r)
        GPUSetPal(target++, MAKECOLORRGB24(r*32, g*32, b*64));*/

  // Set scan-out address
  // One can allocate some memory aligned to 64 bytes and pass
  // the pointer to this function to set that memory to be the display scan-out
  //GPUSetVPage((uint32_t)VRAM); // This is already the default

	// Only on first core
	uint32_t hartid = read_csr(mhartid);
	if (hartid == 0)
	{
		// Ready to handle hardware & software interrupts
		InstallMainISR();
	}

	uint32_t prevVoltage = 0xC004CAFE;
	uint32_t voltage = 0x00000000;
	uint32_t x = 0;

	while(1)
	{
		// Main loop will only wake up at hardware interrupt requests
		//asm volatile("wfi;");

		// While we're awake, also run some voltage measurements
		voltage = (voltage + *XADCPORT)>>1;
		if (prevVoltage != voltage)
		{
			// Clear the line ahead
			for (int k=0;k<240;++k)
				VRAMBYTES[(x+1)%320 + k*320] = 0xFF;
			// Show voltage value
			uint32_t y = (voltage/273)%240;
			VRAMBYTES[x + y*320] = 0x0F;
			if (x==320)
			{
				// Flush data cache so we can see what's happening (entire cache, switch to use one with only one cache page)
				asm volatile( ".word 0xFC000073;");
			}
			// Next column
			x = (x+1)%320;
			prevVoltage = voltage;
		}

		// Flip between vram page 0 and page 1
		//GPUSetVPage((uint32_t)VRAM + (scrollingleds%2)*320*240);

	}

	return 0;
}
