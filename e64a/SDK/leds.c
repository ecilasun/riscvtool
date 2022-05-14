#include "leds.h"

volatile uint32_t *LEDSTATE = (volatile uint32_t* )0x80001030;

uint32_t GetLEDState()
{
	return *LEDSTATE;
}

void SetLEDState(const uint32_t state)
{
	*LEDSTATE = state;
}
