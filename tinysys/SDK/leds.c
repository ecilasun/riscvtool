#include "leds.h"

volatile uint32_t *LEDSTATE = (volatile uint32_t* )0x80001000;

uint32_t LEDGetState()
{
	return *LEDSTATE;
}

void LEDSetState(const uint32_t state)
{
	*LEDSTATE = state;
}
