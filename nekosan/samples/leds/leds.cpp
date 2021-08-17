#include "core.h"
#include "uart.h"
#include "leds.h"

int main()
{
    EchoStr("LED IO Test\n");

    *IO_LEDRW = 0x0;

    uint32_t delay = 0;
    uint32_t tmp = 1;
    while (1) {
        *IO_LEDRW = tmp;
        if ((delay % 131072) == 0)
            tmp = tmp << 1;
        if (tmp==0) tmp = 1;
        delay++;
    }

    return 0;
}
