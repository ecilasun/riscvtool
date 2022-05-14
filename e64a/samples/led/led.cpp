#include <math.h>
#include "core.h"

#include "leds.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	// Turn on all of the LEDs
	SetLEDState(0xFF);

	do{	} while (1);

	return 0;
}