#include "ps2.h"
#include <string.h>

volatile uint32_t *PS2KEYBOARDDATA = (volatile uint32_t* )0x80000050;
volatile uint32_t *PS2KEYBOARDDATAAVAIL = (volatile uint32_t* )0x80000058;

void PS2ScanKeyboard(uint16_t *_keymap)
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
        while (*PS2KEYBOARDDATAAVAIL == 0x0) { }
        code = (*PS2KEYBOARDDATA)&0xFF;
    }

    if (code == 0xF0) // Break code
    {
        brk = 1;
        // Wait for next in sequence
        while (*PS2KEYBOARDDATAAVAIL == 0x0) { }
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

// Simple ringbuffer
// Adapted from DXUT locklesspipe (c) Microsoft

const static uint32_t cbBufferSizeLog2 = 10;
const static uint8_t c_cbBufferSizeLog2 = cbBufferSizeLog2 < 31 ? cbBufferSizeLog2 : 31;
const static uint32_t c_cbBufferSize = ( 1 << c_cbBufferSizeLog2 );
const static uint32_t c_sizeMask = c_cbBufferSize - 1;

// These need to persist in same memory location betwen ROM and loaded ELF
// so that we don't read from wrong space (or not read at all)
static uint8_t *m_ringBuffer;
volatile static uint32_t *m_readOffset;
volatile static uint32_t *m_writeOffset;

void PS2InitRingBuffer()
{
    // Might be missing initialization based on platform, so we use code to initialize
    m_ringBuffer  = (uint8_t *)0x00000100;
    m_readOffset  = (uint32_t *)0x00000010;
    m_writeOffset = (uint32_t *)0x00000020;

    memset((void*)m_readOffset, 0, sizeof(uint32_t));
    memset((void*)m_writeOffset, 0, sizeof(uint32_t));
}

uint32_t __attribute__ ((noinline)) PS2RingBufferRead(void* pvDest, const uint32_t cbDest)
{
    uint32_t readOffset = *m_readOffset;
    const uint32_t writeOffset = *m_writeOffset;

    const uint32_t cbAvailable = writeOffset - readOffset;
    if( cbDest > cbAvailable )
        return 0;

    //EReadWriteBarrier(0);
    asm volatile("": : :"memory"); // Stop compiler reordering

    uint8_t* pbDest = (uint8_t *)pvDest;
    const uint32_t actualReadOffset = readOffset & c_sizeMask;
    uint32_t bytesLeft = cbDest;

    const uint32_t cbTailBytes = bytesLeft < c_cbBufferSize - actualReadOffset ? bytesLeft : c_cbBufferSize - actualReadOffset;
    memcpy( pbDest, m_ringBuffer + actualReadOffset, cbTailBytes );
    bytesLeft -= cbTailBytes;

    //EAssert(bytesLeft == 0, "Item not an exact multiple of ring buffer, this will cause multiple memcpy() calls during Read()");

    readOffset += cbDest;
    *m_readOffset = readOffset;

    return 1;
}

uint32_t __attribute__ ((noinline)) PS2RingBufferWrite(const void* pvSrc, const uint32_t cbSrc)
{
    const uint32_t readOffset = *m_readOffset;
    uint32_t writeOffset = *m_writeOffset;

    const uint32_t cbAvailable = c_cbBufferSize - ( writeOffset - readOffset );
    if( cbSrc > cbAvailable )
        return 0;

    const uint8_t* pbSrc = ( const uint8_t* )pvSrc;
    const uint32_t actualWriteOffset = writeOffset & c_sizeMask;
    uint32_t bytesLeft = cbSrc;

    const uint32_t cbTailBytes = bytesLeft < c_cbBufferSize - actualWriteOffset ? bytesLeft : c_cbBufferSize - actualWriteOffset;
    memcpy(m_ringBuffer + actualWriteOffset, pbSrc, cbTailBytes);
    bytesLeft -= cbTailBytes;

    //EAssert(bytesLeft == 0, "Item not an exact multiple of ring buffer, this will cause multiple memcpy() calls during Write()");

    //EReadWriteBarrier(0);
    asm volatile("": : :"memory"); // Stop compiler reordering

    writeOffset += cbSrc;
    *m_writeOffset = writeOffset;

    return 1;
}
