#include <math.h>
#include "core.h"

#include "leds.h"
#include "uart.h"
#include "gpu.h"

int main()
{
	uint32_t state = 0;
	do{
		SetLEDState(state++);
	} while (1);

	return 0;
}