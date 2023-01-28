#include "uart.h"

#include <math.h> // For abs()

volatile uint8_t *IO_UARTRX     = (volatile uint8_t* ) 0x80000000;
volatile uint8_t *IO_UARTTX     = (volatile uint8_t* ) 0x80000004;
volatile uint8_t *IO_UARTStatus = (volatile uint8_t* ) 0x80000008;

void UARTWrite(const char *_message)
{
    int i = 0;
    while (_message[i]!=0)
        *IO_UARTTX = _message[i++];
}
