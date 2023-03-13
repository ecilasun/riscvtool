#include <inttypes.h>

#define RINGBUFFER_BASE 0x00000200
#define RINGBUFFER_END  0x00000400

void RingBufferReset();
uint32_t RingBufferRead(void* pvDest, const uint32_t cbDest);
uint32_t RingBufferWrite(const void* pvSrc, const uint32_t cbSrc);
