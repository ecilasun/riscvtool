#include "uart.h"

volatile uint8_t *IO_UARTTX = (volatile uint8_t* )0x8000000C;              // UART send data (write)
volatile uint8_t *IO_UARTRX = (volatile uint8_t* )0x80000008;              // UART receive data (read)
volatile uint32_t *IO_UARTRXByteCount = (volatile uint32_t* )0x80000004;   // UART input status (read)
