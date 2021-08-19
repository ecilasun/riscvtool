#include <inttypes.h>

extern volatile uint8_t *IO_UARTRXTX;
extern volatile uint32_t *IO_UARTRXByteAvailable;

void EchoStr(const char *_message);
void EchoHex(const uint32_t i);
void EchoDec(const int32_t i);
