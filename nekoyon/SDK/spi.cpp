#include "spi.h"

volatile uint8_t *IO_SPIRXTX = (volatile uint8_t* )0x8000000C; // SPI read/write port
