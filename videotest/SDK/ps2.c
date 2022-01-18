#include "ps2.h"

volatile uint32_t *PS2KEYBOARD = (volatile uint32_t* )0x20002000;

volatile uint32_t *PS2KEYBOARDDATAAVAIL = (volatile uint32_t* )0x20002008;