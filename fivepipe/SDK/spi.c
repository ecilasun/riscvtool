#include "spi.h"

volatile uint8_t *IO_SPIRXTX = (volatile uint8_t* )0x80000040; // SPI read/write port
