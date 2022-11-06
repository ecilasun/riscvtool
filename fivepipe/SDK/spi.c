#include "spi.h"

volatile uint8_t *IO_SPIRXTX = (volatile uint8_t* )0x80000040; // SPI read/write port
volatile uint8_t *IO_SPICTL  = (volatile uint8_t* )0x80000044; // SPI control register {30'd0, chipselect, poweron}
