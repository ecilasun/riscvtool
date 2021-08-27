#include <inttypes.h>

extern volatile uint8_t *IO_UARTRXTX;
extern volatile uint32_t *IO_UARTRXByteAvailable;
extern volatile uint32_t *IO_UARTTXFIFOFull;

void UARTFlush();

void UARTWrite(const char *_message);
void UARTWriteHex(const uint32_t i);
void UARTWriteDecimal(const int32_t i);
