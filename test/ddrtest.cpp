#include "nekoichi.h"
#include <math.h>
#include <stdio.h>

uint16_t blockRAMTable[1024];

int main()
{
    EchoUART("Clearing extended memory\r\n"); // 0x00000000 - 0x0FFFFFFF
    int i=0;
    uint64_t startclock = ReadClock();
    for (uint32_t m=0x00040000; m<0x04000000; m+=4)
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
    printf("Clearing 64Mbytes took %d ms (0x%.8X)\r\n", (unsigned int)deltams, (unsigned int)deltams);

    int rate = (1024*64*1024) / deltams;
    printf("Zero-write rate is %d Kbytes/sec\r\n", rate);

    printf("Testing 128K extended memory write persistance\r\n"); // 0x00040000 - 0x0FFFFFFF

    const uint32_t stride = 1;

    // Pick a far position in DDR3 memory range
    volatile uint16_t *ddr3mem = (volatile uint16_t *)0x02000000;

    printf("Generating WORD aligned test data\r\n");
    for (uint32_t i=0;i<1024;++i)
        blockRAMTable[i] = (i ^ 0xAFDFBFCF)*13139; // TODO: Use clock as XOR mask here?

    // Copy sine table to DDR3 from block memory
    printf("Copying\r\n");
    for (uint32_t i=0;i<131072;++i)
        ddr3mem[i*stride] = blockRAMTable[i%1024];

    printf("Comparing\r\n");
    int fails = 0;
    // Compare entries at random
    for (uint32_t i=0;i<131072;++i)
    {
        uint32_t idx = (Random()%1024);
        uint32_t V2 = ddr3mem[idx*stride];
        if (blockRAMTable[idx%1024] != V2)
        {
            printf("Mismatch at 0x%.8X, read 0x%.8X, expected 0x%.8X\r\n", (unsigned int)&ddr3mem[idx*stride], (unsigned int)V2, (unsigned int)blockRAMTable[idx%1024]);
            ++fails;
        }
    }

    printf("Extended memory test %s with %d errors.\r\n", fails ? "failed":"passed", fails);

    return 0;
}
