#include <inttypes.h>

// Base address of PS2 keyboard scan code input
extern volatile uint32_t *PS2KEYBOARDDATA;

// Non-zero when we have incoming data
extern volatile uint32_t *PS2KEYBOARDDATAAVAIL;

// Masks for make/break and extended code bits
#define PS2KEYMASK_EXTCODE 0x00000200
#define PS2KEYMASK_BREAKCODE 0x00000100

// Call this from an interrupt service routine to populate
// a key 256 half map in S-RAM
void PS2ScanKeyboard(uint16_t *_keymap);
char PS2ScanToASCII(const uint8_t _code, const uint8_t _uppercase);
