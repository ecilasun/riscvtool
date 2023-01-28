#include "spi.h"

volatile uint8_t *IO_SPIRXTX = (volatile uint8_t* )0x80000020; // SPI read/write port
