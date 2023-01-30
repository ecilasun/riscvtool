#include "dma.h"
#include "core.h"

// NOTE: Writes to this address will end up in the DMA command FIFO
// NOTE: Reads from this address will return DMA operation status
volatile uint32_t *DMAIO = (volatile uint32_t* )0x80008000;

// Simple DMA with aligned start and target addresses, does _16byteBlockCount x 128bit copies (no masking, no byte aligned copies)
void DMACopyBlocks(const uint32_t _sourceAddress16ByteAligned, const uint32_t _targetAddress16ByteAligned, const uint32_t _blockCountInMultiplesOf16bytes)
{
    *DMAIO = DMACMD_SETSOURCE;
    *DMAIO = _sourceAddress16ByteAligned;
    *DMAIO = DMACMD_SETTARGET;
    *DMAIO = _targetAddress16ByteAligned;
    *DMAIO = DMACMD_SETLENGHT;
    *DMAIO = _blockCountInMultiplesOf16bytes;
    *DMAIO = DMACMD_ENQUEUE;
}

uint32_t DMAPending()
{
    return *DMAIO;
}