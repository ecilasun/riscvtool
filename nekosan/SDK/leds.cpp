#include "leds.h"

volatile uint32_t *IO_LEDRW = (volatile uint32_t* )0x8000001C; // LED status read/write port
