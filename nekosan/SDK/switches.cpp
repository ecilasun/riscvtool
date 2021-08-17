#include "switches.h"

volatile uint32_t *IO_SWITCHES = (volatile uint32_t* )0x80000014; // Switch/button status read port
