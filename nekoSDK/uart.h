#include <inttypes.h>

extern volatile uint8_t *IO_UARTRX;
extern volatile uint8_t *IO_UARTTX;
extern volatile uint32_t *IO_UARTRXByteCount;

void EchoUART(const char *_message);
void EchoHex(const uint32_t i);
