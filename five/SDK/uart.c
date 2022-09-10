#include "uart.h"

#include <math.h> // For abs()

volatile uint32_t *IO_UARTRX     = (volatile uint32_t* ) 0x80000000; // Receive fifo
volatile uint32_t *IO_UARTTX     = (volatile uint32_t* ) 0x80000004; // Send fifo
volatile uint32_t *IO_UARTStatus = (volatile uint32_t* ) 0x80000008; // Status (bit#0=rx valid(0:empty,1:havedata),bit#1=rx fifo full,bit#2=tx fifo empty, bit#4=intr enabled, bit#5=overrun, bit#6=frameerr, bit#7=parityerr)
volatile uint32_t *IO_UARTCtl    = (volatile uint32_t* ) 0x8000000C; // Control (write only, bit#4=interrupt enable, bit#1=reset receive fifo, bit#0=reset send fifo, bit#2/3=reserved)

int UARTHasData()
{
    return ((*IO_UARTStatus)&0x00000001);
}

void UARTFlush()
{
    while (UARTHasData()) { asm volatile("nop;"); }
}

uint8_t UARTRead()
{
    if (UARTHasData())
        return (uint8_t)(*IO_UARTRX);
    else
        return 0;
}

void UARTPutChar(const char _char)
{
    // Warning! This might overflow the output FIFO.
    // It is advised to call UARTFlush() before using it.
    *IO_UARTTX = (uint32_t)_char;
}

void UARTWrite(const char *_message)
{
    int forceduartflush = 1;
    // Emit all characters from the input string
    int i = 0;
    while (_message[i]!=0)
    {
        UARTPutChar(_message[i++]);
        if (((++forceduartflush)%768) == 0) // This is a very long send operation, flush before we can resume
            UARTFlush();
    }
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
        uint32_t r = ((i/d)%10)&0x7FFFFFFF;
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
