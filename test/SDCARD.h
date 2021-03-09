#include "inttypes.h"

uint8_t SDIdle();
uint8_t SDCheckVoltageRange(uint32_t &databack);
uint8_t SDCardInit();
uint8_t SDSetBlockSize512();
uint8_t SDReadSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2]);
uint8_t SDCardStartup();

int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);
