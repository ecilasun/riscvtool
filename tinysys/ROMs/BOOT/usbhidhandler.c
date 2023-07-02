#include "usbhidhandler.h"
#include "leds.h"
#include "uart.h"


void HandleUSBA()
{
	uint32_t currLED = LEDGetState();
	LEDSetState(currLED | 0x4);

	// TODO:

	LEDSetState(currLED);
}
