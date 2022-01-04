#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "core.h"
#include "uart.h"
#include "memtest/memtest.h"

#include "../cmdboot.h"

int main()
{
    UARTWrite("\nTesting DDR3 on AXI4-Lite bus\n");

    UARTWrite("Data bus test (0x80000000-0x80003FFF)...");
    int failed = 0;
    for (uint32_t i=0x80000000; i<0x80003FFF; i+=4)
    {
        failed += memTestDataBus((volatile datum*)i);
    }
    UARTWrite(failed == 0 ? "passed (" : "failed (");
    UARTWriteDecimal(failed);
    UARTWrite(" failures)\n");

    UARTWrite("Address bus test (0x80000000-0x80003FFF)...");
    int errortype = 0;
    datum* res = memTestAddressBus((volatile datum*)0x80000000, 16384, &errortype);
    UARTWrite(res == NULL ? "passed\n" : "failed\n");
    if (res != NULL)
    {
        if (errortype == 0)
            UARTWrite("Reason: Address bit stuck high at 0x");
        if (errortype == 1)
            UARTWrite("Reason: Address bit stuck low at 0x");
        if (errortype == 2)
            UARTWrite("Reason: Address bit shorted at 0x");
        UARTWriteHex((unsigned int)res);
        UARTWrite("\n");
    }

    UARTWrite("Memory device test (0x80000000-0x80003FFF)...");
    datum* res2 = memTestDevice((volatile datum *)0x80000000, 16384);
    UARTWrite(res2 == NULL ? "passed\n" : "failed\n");
    if (res2 != NULL)
    {
        UARTWrite("Reason: incorrect value read at 0x");
        UARTWriteHex((unsigned int)res2);
        UARTWrite("\n");
    }

    if ((failed != 0) | (res != NULL) | (res2 != NULL))
      UARTWrite("Scratchpad memory does not appear to be working correctly.\n");

    return 0;
}
