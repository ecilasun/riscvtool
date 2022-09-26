#include <inttypes.h>

extern volatile uint32_t *IO_UARTRX;
extern volatile uint32_t *IO_UARTTX;
extern volatile uint32_t *IO_UARTStatus;
extern volatile uint32_t *IO_UARTCtl;

inline void UARTPutChar(const char _char)
{
    // Warning! This might overflow the output FIFO.
    // It is advised to call UARTFlush() before using it.
    *IO_UARTTX = (uint32_t)_char;
}

void UARTEnableInterrupt(int enable);
void UARTFlush();
int UARTHasData();
void UARTWrite(const char *_message);
uint8_t UARTRead();
void UARTWriteHex(const uint32_t i);
void UARTWriteHexByte(const uint8_t i);
void UARTWriteDecimal(const int32_t i);
