#include "uart.h"

#include <math.h> // For abs()

volatile uint8_t  *IO_UARTRXTX            = (volatile uint8_t* )  0xA0000008; // UART send/receive port (write/read)
volatile uint32_t *IO_UARTRXByteAvailable = (volatile uint32_t* ) 0xA0000004; // UART input status (read)
volatile uint32_t *IO_UARTTXFIFOFull      = (volatile uint32_t* ) 0xA0000000; // UART output status (read)

void UARTFlush()
{
    while (*IO_UARTTXFIFOFull) { asm volatile("nop;"); }
}

void UARTPutChar(const char _char)
{
    // Warning! This might overflow the output FIFO.
    // It is advised to call UARTFlush() before using it.
    *IO_UARTRXTX = _char;
}

void UARTWrite(const char *_message)
{
    // Emit all characters from the input string
    int i = 0;
    while (_message[i]!=0)
    {
        UARTPutChar(_message[i++]);
        if (i%128==0) // time to flush the FIFO
            UARTFlush();
    }
    UARTFlush(); // One last flush at the end
}

void UARTWriteHexByte(const uint8_t i)
{
    const char hexdigits[] = "0123456789ABCDEF";
    char msg[] = "  ";

    msg[0] = hexdigits[((i>>4)%16)];
    msg[1] = hexdigits[(i%16)];

    UARTWrite(msg);
}

void UARTWriteHex(const uint32_t i)
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

    UARTWrite(msg);
}

void UARTWriteDecimal(const int32_t i)
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

    UARTWrite(msg);
}
