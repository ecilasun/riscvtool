#include <stdio.h>

#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
    volatile uint32_t *unmappedmemory = (volatile uint32_t*)0x2020BAAD;
    volatile uint32_t *unmappeddevice = (volatile uint32_t*)0x2020BAAD;

    printf("Testing memory bus write error...\n");
    unmappedmemory[0] = 0xCDCDCDCD;
    unmappedmemory[1] = 0xFEFEFEFE;

    printf("Testing memory bus read error...\n");
    unmappedmemory[1] = unmappedmemory[0];

    printf("Testing device bus write error...\n");
    unmappeddevice[0] = 0xCDCDCDCD;
    unmappeddevice[1] = 0xFEFEFEFE;

    printf("Testing device bus read error...\n");
    unmappeddevice[1] = unmappeddevice[0];

    printf("ERROR: We should not reach here.\n");
    return 0;
}
