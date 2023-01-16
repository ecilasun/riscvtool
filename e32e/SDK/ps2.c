#include "ps2.h"

volatile uint32_t *PS2KEYBOARDDATA = (volatile uint32_t* )0x80001020;
volatile uint32_t *PS2KEYBOARDDATAAVAIL = (volatile uint32_t* )0x80001028;

void PS2ScanKeyboard(uint16_t *_keymap)
{
    uint32_t ext = 0;
    uint32_t brk = 0;
    uint32_t code = 0;

    // Read first code (make, break, or extended)
    if (*PS2KEYBOARDDATAAVAIL)
        code = (*PS2KEYBOARDDATA)&0xFF;

    if (code == 0xE0) // Extended code
    {
        ext = PS2KEYMASK_EXTCODE;
        // Wait for next in sequence
        while (*PS2KEYBOARDDATAAVAIL == 00) { }
        code = (*PS2KEYBOARDDATA)&0xFF;
    }

    if (code == 0xF0) // Break code (otherwise make code, single byte)
    {
        brk = PS2KEYMASK_BREAKCODE;
        // Wait for next in sequence
        while (*PS2KEYBOARDDATAAVAIL == 00) { }
        code = (*PS2KEYBOARDDATA)&0xFF;
    }

    // Merge state and form key code with embedded event bits
    _keymap[code] = ext | brk | code;
}

char scantoasciitable_lowercase[] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 0x0B,  '`',  ' ', // 0
    ' ',  ' ',  ' ',  ' ',  ' ',  'q',  '1',  ' ',  ' ',  ' ',  'z',  's',  'a',  'w',  '2',  ' ', // 1
    ' ',  'c',  'x',  'd',  'e',  '4',  '3',  ' ',  ' ',  ' ',  'v',  'f',  't',  'r',  '5',  ' ', // 2
    ' ',  'n',  'b',  'h',  'g',  'y',  '6',  ' ',  ' ',  ' ',  'm',  'j',  'u',  '7',  '8',  ' ', // 3
    ' ',  ',',  'k',  'i',  'o',  '0',  '9',  ' ',  ' ',  '.',  '/',  'l',  ';',  'p',  '-',  ' ', // 4
    ' ',  ' ', '\'',  ' ',  '[',  '=',  ' ',  ' ',  ' ',  ' ', 0x0D,  ']',  ' ', '\\',  ' ',  ' ', // 5
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

char scantoasciitable_uppercase[] = {
//   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ', 0x0B,  '`',  ' ', // 0
    ' ',  ' ',  ' ',  ' ',  ' ',  'Q',  '!',  ' ',  ' ',  ' ',  'Z',  'S',  'A',  'W',  '@',  ' ', // 1
    ' ',  'C',  'X',  'D',  'E',  '$',  '#',  ' ',  ' ',  ' ',  'V',  'F',  'T',  'R',  '%',  ' ', // 2
    ' ',  'N',  'B',  'H',  'G',  'Y',  '^',  ' ',  ' ',  ' ',  'M',  'J',  'U',  '7',  '*',  ' ', // 3
    ' ',  '<',  'K',  'I',  'O',  ')',  '(',  ' ',  ' ',  '>',  '?',  'l',  ':',  'P',  '_',  ' ', // 4
    ' ',  ' ',  '"',  ' ',  '{',  '+',  ' ',  ' ',  ' ',  ' ', 0x0D,  '}',  ' ',  '|',  ' ',  ' ', // 5
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

char PS2ScanToASCII(const uint8_t _code, const uint8_t _uppercase)
{
    return _uppercase ? scantoasciitable_uppercase[_code] : scantoasciitable_lowercase[_code];
}
