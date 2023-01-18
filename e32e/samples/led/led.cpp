#include <math.h>
#include "core.h"

#include "leds.h"

int main()
{
	uint32_t state = 0;
	do{
		LEDSetState(state++);
		E32Sleep(100);
	} while (1);

	return 0;
}