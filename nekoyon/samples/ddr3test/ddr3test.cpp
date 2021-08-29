#include "core.h"
#include "uart.h"
#include <math.h>
#include <stdio.h>

#include "memtest/memtest.h"

int main()
{
    UARTWrite("Clearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF
    int i=0;
    uint64_t startclock = ReadClock();
    for (uint32_t m=0x01000000; m<0x03000000; m+=4)
    {
        *((uint32_t*)m) = 0x00000000;
        if ((m!=0) && ((m%0x100000) == 0))
        {
            ++i;
            UARTWriteDecimal(i);
            UARTWrite(" Mbytes cleared @");
            UARTWriteHex((unsigned int)m);
            UARTWrite("\n");

        }
    }

    uint64_t endclock = ReadClock();
    uint32_t deltams = ClockToMs(endclock-startclock);
    UARTWrite("Clearing 32Mbytes took ");
    UARTWriteDecimal((unsigned int)deltams);
    UARTWrite(" ms\n");

    int rate = (1024*32*1024) / deltams;
    UARTWrite("Zero-write rate is ");
    UARTWriteDecimal(rate);
    UARTWrite(" Kbytes/sec\n");

    UARTWrite("\n-------------MemTest--------------\n");
    UARTWrite("Copyright (c) 2000 by Michael Barr\n");
    UARTWrite("----------------------------------\n");

    UARTWrite("Walking-1s test (0x01000000-0x01000F00)\n");
    int failed = 0;
    for (uint32_t i=0x01000000; i<0x01000F00; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    UARTWriteDecimal(failed);
    UARTWrite(" failures\n");

    UARTWrite("Address bus test (0x01000000-4Kbytes)\n");
    datum* res = memTestAddressBus((volatile datum*)0x01000000, 4096);
    UARTWrite(res == NULL ? "passed" : "failed");
    UARTWrite("\n");
    if (res != NULL)
    {
        UARTWrite("Reason: address aliasing problem at 0x");
        UARTWriteHex((unsigned int)res);
        UARTWrite("\n");
    }

    UARTWrite("Memory device test (0x01000000-4Kbytes)\n");
    datum* res2 = memTestDevice((volatile datum *)0x01000000, 4096);
    UARTWrite(res2 == NULL ? "passed" : "failed");
    UARTWrite("\n");
    if (res2 != NULL)
    {
        UARTWrite("Reason: incorrect value read at 0x");
        UARTWriteHex((unsigned int)res2);
        UARTWrite("\n");
    }

    // Stay here as we don't have anywhere to go back to
    while (1) { }

    return 0;
}
