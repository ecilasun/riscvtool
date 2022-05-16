#include <inttypes.h>

extern volatile uint8_t *IO_UARTRX;
extern volatile uint8_t *IO_UARTTX;
extern volatile uint8_t *IO_UARTStatus;

void UARTPutChar(const char _char);
void UARTWrite(const char *_message);
