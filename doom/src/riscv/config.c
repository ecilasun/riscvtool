#include "config.h"

volatile unsigned char *IO_UARTRXTX           = (volatile unsigned char* )  0x20000008; // UART send/receive port (write/read)
volatile unsigned int *IO_UARTRXByteAvailable = (volatile unsigned int* ) 0x20000004; // UART input status (read)
volatile unsigned int *IO_UARTTXFIFOFull      = (volatile unsigned int* ) 0x20000000; // UART output status (read)

uint64_t ReadClock()
{
   uint32_t clockhigh, clocklow, tmp;

   asm (
      "1:\n"
      "rdtimeh %0\n"
      "rdtime %1\n"
      "rdtimeh %2\n"
      "bne %0, %2, 1b\n"
      : "=&r" (clockhigh), "=&r" (clocklow), "=&r" (tmp)
   );

   unsigned long long clock = (((unsigned long long)clockhigh)<<32) | clocklow;

   return clock;
}

uint64_t ClockToMs(uint64_t clock)
{
   return clock / 10000;
}

uint64_t ClockToUs(uint64_t clock)
{
   return clock / 10;
}
