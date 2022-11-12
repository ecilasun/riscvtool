#include <stdio.h>

#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
    printf("Testing bus error...\n");

    uint32_t *nomansland = (uint32_t*)0x7FEDBAAD;

    nomansland[0] = 0xCDCDCDCD;
    nomansland[1] = 0xFEFEFEFE;

    return 0;
}
