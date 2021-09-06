#include "config.h"

volatile unsigned char *IO_SPIRXTX = (volatile unsigned char* )0x8000000C;       // SPU send data (write)
volatile unsigned char *IO_UARTRXTX = (volatile unsigned char* )0x80000008;          // UART send data (write)
volatile unsigned int *IO_UARTRXByteCount = (volatile unsigned int* )0x80000004;   // UART input status (read)
volatile unsigned int *IO_UARTTXFifoFull = (volatile unsigned int* )0x80000000;
volatile unsigned int *PRAMStart = (volatile unsigned int* )0x30000000;
volatile unsigned int *GRAMStart = (volatile unsigned int* )0x20000000;
volatile unsigned int *SRAMStart = (volatile unsigned int* )0x10000000;

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
