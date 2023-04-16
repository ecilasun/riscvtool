#include "basesystem.h"
#include "uart.h"
#include "audio.h"
#include <stdlib.h>

int main()
{
    UARTWrite("Sigma-delta audio output test\n");

    do
    {
        *IO_AUDIOOUT = rand();
    } while (1);
    

    return 0;
}
