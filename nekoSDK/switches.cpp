#include "switches.h"

volatile uint32_t *IO_SwitchByteCount = (volatile uint32_t* )0x8000001C;   // Switch state byte count (read)
volatile uint8_t *IO_SwitchState = (volatile uint8_t* )0x80000018;         // Device switch states (read)
