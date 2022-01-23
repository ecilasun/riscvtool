#include "ringbuffer.h"
#include <string.h>

// Simple ringbuffer
// Adapted from DXUT locklesspipe (c) Microsoft

const static uint32_t cbBufferSizeLog2 = 10;

volatile static uint32_t m_readOffset = 0;
volatile static uint32_t m_writeOffset = 0;
const static uint8_t c_cbBufferSizeLog2 = cbBufferSizeLog2 < 31 ? cbBufferSizeLog2 : 31;
const static uint32_t c_cbBufferSize = ( 1 << c_cbBufferSizeLog2 );
const static uint32_t c_sizeMask = c_cbBufferSize - 1;

uint32_t RingBufferRead(uint8_t *ringbuffer, void* pvDest, const uint32_t cbDest)
{
    uint32_t readOffset = m_readOffset;
    const uint32_t writeOffset = m_writeOffset;

    const uint32_t cbAvailable = writeOffset - readOffset;
    if( cbDest > cbAvailable )
        return 0;

    //EReadWriteBarrier(0);
    asm volatile ("nop;"); // Stop compiler reordering

    uint8_t* pbDest = (uint8_t *)pvDest;
    const uint32_t actualReadOffset = readOffset & c_sizeMask;
    uint32_t bytesLeft = cbDest;

    const uint32_t cbTailBytes = bytesLeft < c_cbBufferSize - actualReadOffset ? bytesLeft : c_cbBufferSize - actualReadOffset;
    memcpy( pbDest, ringbuffer + actualReadOffset, cbTailBytes );
    bytesLeft -= cbTailBytes;

    //EAssert(bytesLeft == 0, "Item not an exact multiple of ring buffer, this will cause multiple memcpy() calls during Read()");

    readOffset += cbDest;
    m_readOffset = readOffset;

    return 1;
}

uint32_t RingBufferWrite(uint8_t *ringbuffer, const void* pvSrc, const uint32_t cbSrc)
{
    const uint32_t readOffset = m_readOffset;
    uint32_t writeOffset = m_writeOffset;

    const uint32_t cbAvailable = c_cbBufferSize - ( writeOffset - readOffset );
    if( cbSrc > cbAvailable )
        return 0;

    const uint8_t* pbSrc = ( const uint8_t* )pvSrc;
    const uint32_t actualWriteOffset = writeOffset & c_sizeMask;
    uint32_t bytesLeft = cbSrc;

    const uint32_t cbTailBytes = bytesLeft < c_cbBufferSize - actualWriteOffset ? bytesLeft : c_cbBufferSize - actualWriteOffset;
    memcpy(ringbuffer + actualWriteOffset, pbSrc, cbTailBytes);
    bytesLeft -= cbTailBytes;

    //EAssert(bytesLeft == 0, "Item not an exact multiple of ring buffer, this will cause multiple memcpy() calls during Write()");

    //EReadWriteBarrier(0);
    asm volatile ("nop;"); // Stop compiler reordering

    writeOffset += cbSrc;
    m_writeOffset = writeOffset;

    return 1;
}
