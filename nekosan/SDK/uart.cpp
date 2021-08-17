#include "uart.h"

#include <math.h> // For abs()

volatile uint8_t *IO_UARTRXTX = (volatile uint8_t* )0x80000008;            // UART send/receive port (write/read)
volatile uint32_t *IO_UARTRXByteCount = (volatile uint32_t* )0x80000004;   // UART input status (read)

void EchoStr(const char *_message)
{
   int i=0;
   while (_message[i]!=0)
   {
      *IO_UARTRXTX = _message[i];
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
   EchoStr(msg);
}

void EchoDec(const int32_t i)
{
    const char digits[] = "0123456789";
    char msg[] = "                   ";

    int d = 1000000000;
    uint32_t enableappend = 0;
    uint32_t m = 0;
    if (i<0)
        msg[m++] = '-';
    for (int c=0;c<10;++c)
    {
        uint32_t r = abs(i/d)%10;
        // Ignore preceeding zeros
        if ((r!=0) || enableappend || d==1)
        {
            enableappend = 1; // Rest of the digits can be appended
            msg[m++] = digits[r];
        }
        d = d/10;
    }
    msg[m] = 0;

    EchoStr(msg);
}
