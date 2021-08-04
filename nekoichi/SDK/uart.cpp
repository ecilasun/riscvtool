#include "uart.h"

volatile uint8_t *IO_UARTTX = (volatile uint8_t* )0x8000000C;              // UART send data (write)
volatile uint8_t *IO_UARTRX = (volatile uint8_t* )0x80000008;              // UART receive data (read)
volatile uint32_t *IO_UARTRXByteCount = (volatile uint32_t* )0x80000004;   // UART input status (read)

void EchoUART(const char *_message)
{
   int i=0;
   while (_message[i]!=0)
   {
      *IO_UARTTX = _message[i];
      ++i;
   }
}

void EchoHex(const uint32_t i)
{
   const char hexdigits[] = "0123456789ABCDEF";
   char msg[] = "        ";
   msg[0] = hexdigits[((i>>28)%16)];
   msg[1] = hexdigits[((i>>24)%16)];
   msg[2] = hexdigits[((i>>20)%16)];
   msg[3] = hexdigits[((i>>16)%16)];
   msg[4] = hexdigits[((i>>12)%16)];
   msg[5] = hexdigits[((i>>8)%16)];
   msg[6] = hexdigits[((i>>4)%16)];
   msg[7] = hexdigits[(i%16)];
   EchoUART(msg);
}
