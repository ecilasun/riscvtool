#include "inttypes.h"

int SDCardStartup();
//void SDCardControl(int power_cs_n);
int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);
int SDWriteMultipleBlocks(const uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);