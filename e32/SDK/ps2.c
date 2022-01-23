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

char scantoasciitable_lowercase[] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 0x0B,  '`',  ' ', // 0
    ' ',  ' ',  ' ',  ' ',  ' ',  'q',  '1',  ' ',  ' ',  ' ',  'z',  's',  'a',  'w',  '2',  ' ', // 1
    ' ',  'c',  'x',  'd',  'e',  '4',  '3',  ' ',  ' ',  ' ',  'v',  'f',  't',  'r',  '5',  ' ', // 2
    ' ',  'n',  'b',  'h',  'g',  'y',  '6',  ' ',  ' ',  ' ',  'm',  'j',  'u',  '7',  '8',  ' ', // 3
    ' ',  ',',  'k',  'i',  'o',  '0',  '9',  ' ',  ' ',  '.',  '/',  'l',  ';',  'p',  '-',  ' ', // 4
    ' ',  ' ', '\'',  ' ',  '[',  '=',  ' ',  ' ',  ' ',  ' ', 0x13,  ']',  ' ', '\\',  ' ',  ' ', // 5
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 0x08,  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // 6
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 0x1B,  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // 7
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // 8
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // 9
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // A
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // B
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // C
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // D
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', // E
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' '  // F
};

char ScanToASCII(const uint8_t _code)
{
    return scantoasciitable_lowercase[_code];
}
