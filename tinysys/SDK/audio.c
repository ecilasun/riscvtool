#include "spi.h"

volatile uint8_t *IO_AUDIOTX = (volatile uint8_t* )0x80008000; // SPI write port
