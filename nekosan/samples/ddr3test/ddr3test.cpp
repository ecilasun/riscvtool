#include "core.h"
#include "uart.h"
#include <math.h>
#include <stdio.h>

#include "memtest/memtest.h"

int main()
{
    EchoStr("Clearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF
    int i=0;
    uint64_t startclock = ReadClock();
    for (uint32_t m=0x01000000; m<0x03000000; m+=4)
    {
        *((uint32_t*)m) = 0x00000000;
        if ((m!=0) && ((m%0x100000) == 0))
        {
            ++i;
            EchoDec(i);
            EchoStr(" Mbytes cleared @");
            EchoHex((unsigned int)m);
            EchoStr("\n");

        }
    }

    uint64_t endclock = ReadClock();
    uint32_t deltams = ClockToMs(endclock-startclock);
    EchoStr("Clearing 32Mbytes took ");
    EchoDec((unsigned int)deltams);
    EchoStr(" ms\n");

    int rate = (1024*32*1024) / deltams;
    EchoStr("Zero-write rate is ");
    EchoDec(rate);
    EchoStr(" Kbytes/sec\n");

    EchoStr("\n-------------MemTest--------------\n");
    EchoStr("Copyright (c) 2000 by Michael Barr\n");
    EchoStr("----------------------------------\n");

    EchoStr("Walking-1s test (0x01000000-0x01000F00)\n");
    int failed = 0;
    for (uint32_t i=0x01000000; i<0x01000F00; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    EchoDec(failed);
    EchoStr(" failures\n");

    EchoStr("Address bus test (0x01000000-4Kbytes)\n");
    datum* res = memTestAddressBus((volatile datum*)0x01000000, 4096);
    EchoStr(res == NULL ? "passed" : "failed");
    EchoStr("\n");
    if (res != NULL)
    {
        EchoStr("Reason: address aliasing problem at 0x");
        EchoHex((unsigned int)res);
        EchoStr("\n");
    }

    EchoStr("Memory device test (0x01000000-4Kbytes)\n");
    datum* res2 = memTestDevice((volatile datum *)0x01000000, 4096);
    EchoStr(res2 == NULL ? "passed" : "failed");
    EchoStr("\n");
    if (res2 != NULL)
    {
        EchoStr("Reason: incorrect value read at 0x");
        EchoHex((unsigned int)res2);
        EchoStr("\n");
    }

    // Stay here as we don't have anywhere to go back to
    while (1) { }

    return 0;
}
