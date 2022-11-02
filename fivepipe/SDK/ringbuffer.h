#include <inttypes.h>

void InitRingBuffer();
uint32_t RingBufferRead(uint8_t *ringbuffer, void* pvDest, const uint32_t cbDest);
uint32_t RingBufferWrite(uint8_t *ringbuffer, const void* pvSrc, const uint32_t cbSrc);
