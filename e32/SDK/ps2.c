#include "ps2.h"

volatile uint32_t *PS2KEYBOARDDATA = (volatile uint32_t* )0x20002000;

volatile uint32_t *PS2KEYBOARDDATAAVAIL = (volatile uint32_t* )0x20002008;

void ScanKeyboard(uint16_t *_keymap)
{
    uint32_t ext = 0;
    uint32_t brk = 0;
    uint32_t code = 0x00;

    // Read first code
    if (*PS2KEYBOARDDATAAVAIL)
        code = (*PS2KEYBOARDDATA)&0xFF;

    if (code == 0xE0) // Extended code
    {
        ext = 2;
        // Wait for next in sequence
        while (*PS2KEYBOARDDATAAVAIL == 00) { }
        code = (*PS2KEYBOARDDATA)&0xFF;
    }

    if (code == 0xF0) // Break code
    {
        brk = 1;
        // Wait for next in sequence
        while (*PS2KEYBOARDDATAAVAIL == 00) { }
        code = (*PS2KEYBOARDDATA)&0xFF;
    }

    // Merge state and key code
    _keymap[code] = (ext<<9) | (brk<<8) | (code);
}
