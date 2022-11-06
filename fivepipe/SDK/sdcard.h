#include "inttypes.h"

void SDCardControl(uint8_t controlBits);
int SDCardStartup();
int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);
int SDWriteMultipleBlocks(const uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);