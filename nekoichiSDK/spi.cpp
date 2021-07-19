#include "spi.h"

volatile uint8_t *IO_SPIOutput = (volatile uint8_t* )0x80000014;           // SPU send data (write)
volatile uint8_t *IO_SPIInput = (volatile uint8_t* )0x80000010;            // SPI receive data (read)
