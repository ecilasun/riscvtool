#include "ps2.h"

volatile uint32_t *PS2KEYBOARDDATA = (volatile uint32_t* )0x20002000;

volatile uint32_t *PS2KEYBOARDDATAAVAIL = (volatile uint32_t* )0x20002008;

void ScanKeyboard(uint8_t *_keymap)
{
    uint32_t extended = 0;
    uint32_t isup = 0;

    uint32_t keydata = 0x00;

    if (*PS2KEYBOARDDATAAVAIL)
        keydata = (*PS2KEYBOARDDATA)&0xFF;

    if (keydata == 0xE0) // Extended, scan one more
    {
        extended = 2;
        if (*PS2KEYBOARDDATAAVAIL)
            keydata = (*PS2KEYBOARDDATA)&0xFF; // Up state or key code
    }

    if (keydata == 0xF0) // Up state, scan one more for the key code
    {
        isup = 1;
        if (*PS2KEYBOARDDATAAVAIL)
            keydata = (*PS2KEYBOARDDATA)&0xFF;
    }

    if (keydata == 0x00) // Nothing to do
        return;

    // This is to be placed into S-RAM and ScanKeyboard called from an ISR
    _keymap[keydata] = (extended) | (isup);
}

// For future use
/*enum EKeySymbol
{
    // Key symbols
    EKeyNone,
    EKeyESC,                // 76
    EKeyF1,                 // 05
    EKeyF2,                 // 06
    EKeyF3,                 // 04
    EKeyF4,                 // 0C
    EKeyF5,                 // 03
    EKeyF6,                 // 0B
    EKeyF7,                 // 83
    EKeyF8,                 // 0A
    EKeyF9,                 // 01
    EKeyF10,                // 09
    EKeyF11,                // 78
    EKeyF12,                // 07
    EKeyTilde,              // 0E
    EKey1,                  // 16
    EKey2,                  // 1E
    EKey3,                  // 26
    EKey4,                  // 25
    EKey5,                  // 2E
    EKey6,                  // 36
    EKey7,                  // 3D
    EKey8,                  // 3E
    EKey9,                  // 46
    EKey0,                  // 45
    EKeyMinus,              // 4E
    EKeyEquals,             // 55
    EKeyBackspace,          // 66
    EKeyTab,                // 0D
    EKeyQ,                  // 15
    EKeyW,                  // 1D
    EKeyE,                  // 24
    EKeyR,                  // 2D
    EKeyT,                  // 2C
    EKeyY,                  // 35
    EKeyU,                  // 3C
    EKeyI,                  // 43
    EKeyO,                  // 44
    EKeyP,                  // 4D
    EKeyLeftBracket,        // 54
    EKeyRightBracket,       // 5B
    EKeyBackSlash,          // 5D
    EKeyCapsLock,           // 58
    EKeyA,                  // 1C
    EKeyS,                  // 1B
    EKeyD,                  // 23
    EKeyF,                  // 2B
    EKeyG,                  // 34
    EKeyH,                  // 33
    EKeyJ,                  // 3B
    EKeyK,                  // 42
    EKeyL,                  // 4B
    EKeySemicolumn,         // 4C
    EKeySingleQuote,        // 52
    EKeyEnter,              // 5A
    EKeyLeftShift,          // 12
    EKeyZ,                  // 1Z
    EKeyX,                  // 22
    EKeyC,                  // 21
    EKeyV,                  // 2A
    EKeyB,                  // 32
    EKeyN,                  // 31
    EKeyM,                  // 3A
    EKeyComma,              // 41
    EKeyPeriod,             // 49
    EKeySlash,              // 4A
    EKeyRightShift,         // 59
    EKeyLeftControl,        // 14
    EKeyWindows,            // E0 1F
    EKeyLeftAlt,            // 11
    EKeySpace,              // 29
    EKeyRightAlt,           // E0 11 - E0 F0 11
    EKeyWinMenu,            // E0 2F - E0 F0 2F
    EKeyRightControl,       // E0 14 - E0 F0 14
    EKeyUpArrow,            // 75 - F0 75
    EKeyLeftArrow,          // 6B - F0 6B
    EKeyDownArrow,          // 72 - F0 72
    EKeyRightArrow,         // E0 74 - E0 F0 74
};*/