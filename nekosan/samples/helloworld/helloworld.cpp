#include "core.h"
#include "uart.h"

int main()
{
    EchoStr("Hello, world!\n");

    // Stay here, as we don't have anywhere else to go back to
    while (1) { }

    return 0;
}
