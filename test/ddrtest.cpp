#include "nekoichi.h"
#include <math.h>

uint16_t blockRAMTable[1024];

int main()
{
    EchoUART("Testing extended memory\r\n"); // 0x00040000 - 0x0FFFFFFF

    const uint32_t stride = 1;

    // Pick a far position in memory towards the end of DDR3 range
    volatile uint16_t *ddr3mem = (volatile uint16_t *)0x0D000000;

    EchoUART("Generating WORD aligned test data\r\n");
    for (uint32_t i=0;i<1024;++i)
        blockRAMTable[i] = (i ^ 0xAFDFBFCF)*13139; // TODO: Use clock as XOR mask here?

    // Copy sine table to DDR3 from block memory
    EchoUART("Copying\r\n");
    for (uint32_t i=0;i<1024;++i)
        ddr3mem[i*stride] = blockRAMTable[i];

    EchoUART("Comparing\r\n");
    int fails = 0;
    // Compare entries at random
    for (uint32_t i=0;i<1024;++i)
    {
        uint32_t idx = (Random()%1024);
        uint32_t V2 = ddr3mem[idx*stride];
        if (blockRAMTable[idx] != V2)
        {
            //EchoUART("Mismatch at 0x");
            EchoHex((uint32_t)&ddr3mem[idx*stride]);
            EchoUART(". Read=0x");
            EchoHex(V2);
            EchoUART(", Original=0x");
            EchoHex(blockRAMTable[idx]);
            EchoUART("\r\n");
            ++fails;
        }
    }

    EchoUART("Extended memory test ");
    if (fails)
    {
        EchoUART("FAILED 0x");
        EchoHex(fails);
        EchoUART(" times.\r\n");
    }
    else
        EchoUART("PASSED\r\n");

    return 0;
}