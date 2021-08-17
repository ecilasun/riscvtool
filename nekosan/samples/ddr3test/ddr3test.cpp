#include "core.h"
#include "uart.h"
#include <math.h>
#include <stdio.h>

#include "memtest/memtest.h"

int main()
{
    printf("Clearing extended memory\n"); // 0x00000000 - 0x0FFFFFFF
    int i=0;
    uint64_t startclock = ReadClock();
    for (uint32_t m=0x01000000; m<0x03000000; m+=4)
    {
        *((uint32_t*)m) = 0x00000000;
        if ((m!=0) && ((m%0x100000) == 0))
        {
            ++i;
            printf("%d Mbytes cleared @%.8X\r\n", i, (unsigned int)m);
        }
    }

    uint64_t endclock = ReadClock();
    uint32_t deltams = ClockToMs(endclock-startclock);
    printf("\nClearing 32Mbytes took %d ms (0x%.8X)\n", (unsigned int)deltams, (unsigned int)deltams);

    int rate = (1024*32*1024) / deltams;
    printf("Zero-write rate is %d Kbytes/sec\n", rate);

    printf("\n-------------MemTest--------------\n");
    printf("Copyright (c) 2000 by Michael Barr\n");
    printf("----------------------------------\n");

    printf("Walking-1s test (0x01000000-0x01000F00)\n");
    int failed = 0;
    for (uint32_t i=0x01000000; i<0x01000F00; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    printf("%d failures\n", failed);

    printf("Address bus test (0x01000000-4Kbytes)\n");
    datum* res = memTestAddressBus((volatile datum*)0x01000000, 4096);
    printf("%s\n", res == NULL ? "passed" : "failed");
    if (res != NULL)
        printf("Reason: address aliasing problem at %.8X\n", (unsigned int)res);

    printf("Memory device test (0x01000000-4Kbytes)\n");
    datum* res2 = memTestDevice((volatile datum *)0x01000000, 4096);
    printf("%s\n", res2 == NULL ? "passed" : "failed");
    if (res2 != NULL)
        printf("Reason: incorrect value read at %.8X\n", (unsigned int)res2);

    // Stay here as we don't have anywhere to go back to
    while (1) { }

    return 0;
}
