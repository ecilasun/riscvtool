// Boot ROM

#include "rvcrt0.h"
//#include "uart.h"
#include "leds.h"

int main()
{
	//UARTWrite("\033[H\033[0m\033[2J\nLemur v0.1 (c) 2023 Engin Cilasun\n");

	// Loop forever
	uint32_t ledout = 0;
	while(1) {
		LEDSetState((ledout>>12)&0xFF);
		++ledout;
	}

	return 0;
}
