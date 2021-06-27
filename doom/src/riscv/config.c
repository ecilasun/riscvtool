#include "config.h"

volatile unsigned int *IO_AudioOutput = (volatile unsigned int* )0x80000020;       // Two 16bit stereo samples to output (31:16->Right, 15:0->Left)
volatile unsigned int *IO_SwitchByteCount = (volatile unsigned int* )0x8000001C;   // Switch state byte count (read)
volatile unsigned char *IO_SwitchState = (volatile unsigned char* )0x80000018;     // Device switch states (read)
volatile unsigned char *IO_SPIOutput = (volatile unsigned char* )0x80000014;       // SPU send data (write)
volatile unsigned char *IO_SPIInput = (volatile unsigned char* )0x80000010;        // SPI receive data (read)
volatile unsigned char *IO_UARTTX = (volatile unsigned char* )0x8000000C;          // UART send data (write)
volatile unsigned char *IO_UARTRX = (volatile unsigned char* )0x80000008;          // UART receive data (read)
volatile unsigned int *IO_UARTRXByteCount = (volatile unsigned int* )0x80000004;   // UART input status (read)
volatile unsigned int *IO_GPUFIFO = (volatile unsigned int* )0x80000000;           // GPU control FIFO

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
