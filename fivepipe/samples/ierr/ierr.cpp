#include <stdio.h>

#include "core.h"
#include "uart.h"
#include "gpu.h"

int main()
{
    printf("Testing instruction error...\n");

    asm volatile(
        ".word 0x00000000;" // Illegal instruction
    );
    
    printf("ERROR: We should not reach here.\n");
    return 0;
}
