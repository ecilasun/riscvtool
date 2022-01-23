#include <inttypes.h>

// Base address of PS2 keyboard scan code input
extern volatile uint32_t *PS2KEYBOARDDATA;

// Non-zero when we have incoming data
extern volatile uint32_t *PS2KEYBOARDDATAAVAIL;

// Call this from an interrupt service routine to populate
// a key 256 byte map in S-RAM
void ScanKeyboard(uint8_t *_keymap);
