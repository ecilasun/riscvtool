// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "rvcrt0.h"

#include "core.h"
#include "uart.h"

int main()
{
   UARTWrite("E_32\n(c)Engin Cilasun 2022\n");

   while(1)
   {
      // Echo back incoming bytes
      if (*IO_UARTRXByteAvailable)
      {
         // Read incoming character
         uint8_t incoming = *IO_UARTRXTX;
         // Write back to UART
         UARTFlush();
         *IO_UARTRXTX = incoming;
      }
   }

   return 0;
}
