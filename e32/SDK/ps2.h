#include <inttypes.h>

// Base address of PS2 keyboard scan code input
extern volatile uint32_t *PS2KEYBOARD;

// Non-zero when we have incoming data
extern volatile uint32_t *PS2KEYBOARDDATAAVAIL;
