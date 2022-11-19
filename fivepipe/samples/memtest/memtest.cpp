#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "memtest/memtest.h"

int main()
{
    printf("\nTesting DDR3 on AXI4 bus\n");

    printf("Clearing extended memory (0x010000000-0x1F000000)\n");
    uint64_t startclock = E32ReadTime();
    for (uint32_t m=0x01000000; m<0x1F000000; m+=4)
        *((volatile uint32_t*)m) = 0x00000000;

    uint64_t endclock = E32ReadTime();
    uint32_t deltams = ClockToMs(endclock-startclock);
    printf("Clearing 480Mbytes took %d ms\n", (unsigned int)deltams);

    int rate = (1024*480*1024) / deltams;
    printf("Zero-write rate is %d Kbytes/sec\n", rate);

    printf("\n-------------MemTest--------------\n");
    printf("Copyright (c) 2000 by Michael Barr\n");
    printf("----------------------------------\n");

    printf("Data bus test (0x01000000-0x1F000000)...\n");
    int failed = 0;
    for (uint32_t i=0x01000000; i<0x1F000000; i+=4)
        failed += memTestDataBus((volatile datum*)i);

    printf("%s (%d failures)\n", failed == 0 ? "passed" : "failed", failed);

    printf("Address bus test (0x01000000-0x1F000000)...\n");
    int errortype = 0;
    datum* res = memTestAddressBus((volatile datum*)0x01000000, 0x1F000000-0x01000000, &errortype);
    printf(res == NULL ? "passed\n" : "failed\n");
    if (res != NULL)
    {
        if (errortype == 0)
            printf("Reason: Address bit stuck high at 0x%8X\n", (unsigned int)res);
        if (errortype == 1)
            printf("Reason: Address bit stuck low at 0x%8X\n", (unsigned int)res);
        if (errortype == 2)
            printf("Reason: Address bit shorted at 0x%8X\n", (unsigned int)res);
    }

    printf("Memory device test (0x01000000-0x1F000000)...");
    datum* res2 = memTestDevice((volatile datum *)0x01000000, 0x1F000000-0x01000000);
    printf(res2 == NULL ? "passed\n" : "failed\n");
    if (res2 != NULL)
        printf("Reason: incorrect value read at 0x%.8X\n", (unsigned int)res2);

    if ((failed != 0) | (res != NULL) | (res2 != NULL))
      printf("Memory device does not appear to be working correctly.\n");

    return 0;
}
