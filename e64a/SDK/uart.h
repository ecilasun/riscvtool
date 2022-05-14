#include <inttypes.h>

extern volatile uint32_t *IO_UARTRX;
extern volatile uint32_t *IO_UARTTX;
extern volatile uint32_t *IO_UARTStatus;
extern volatile uint32_t *IO_UARTCtl;

void UARTFlush();
int UARTHasData();
void UARTPutChar(const char _char);
void UARTWrite(const char *_message);
uint8_t UARTRead();
void UARTWriteHex(const uint32_t i);
void UARTWriteHexByte(const uint8_t i);
void UARTWriteDecimal(const int32_t i);
