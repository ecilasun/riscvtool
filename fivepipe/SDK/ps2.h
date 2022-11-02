#include <inttypes.h>

// Base address of PS2 keyboard scan code input
extern volatile uint32_t *PS2KEYBOARDDATA;

// Non-zero when we have incoming data
extern volatile uint32_t *PS2KEYBOARDDATAAVAIL;

// Call this from an interrupt service routine to populate
// a key 256 half map in S-RAM
void ScanKeyboard(uint16_t *_keymap);
char ScanToASCII(const uint8_t _code, const uint8_t _uppercase);

void InitRingBuffer();
uint32_t RingBufferRead(void* pvDest, const uint32_t cbDest);
uint32_t RingBufferWrite(const void* pvSrc, const uint32_t cbSrc);
