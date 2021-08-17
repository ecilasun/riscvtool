#include "switches.h"

volatile uint32_t *IO_SWITCHREADY = (volatile uint32_t* )0x80000014;    // Switch/button ready port
volatile uint32_t *IO_SWITCHES = (volatile uint32_t* )0x80000010;       // Switch/button state read port
