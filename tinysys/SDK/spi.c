#include "spi.h"

volatile uint8_t *IO_SPIRXTX = (volatile uint8_t* )0x80003000; // SPI read/write port
volatile uint8_t *IO_CARDDETECT = (volatile uint8_t* )0x80003004; // SDCard insert/remove detect