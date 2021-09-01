#include "core.h"
#include "uart.h"

uint32_t asuint(const float F)
{
    float K = F;
    uint32_t *Kasu32 = (uint32_t*)(&K);

    return *Kasu32;
}

int main()
{
    float a = 1.3124145f;
    float b = 553.538131f;

    float c = a / b;
    UARTWriteHex(asuint(a));
    UARTWrite("/");
    UARTWriteHex(asuint(b));
    UARTWrite("=");
    UARTWriteHex(asuint(c));
    UARTWrite("\n");

    float d = a * b;
    UARTWriteHex(asuint(a));
    UARTWrite("*");
    UARTWriteHex(asuint(b));
    UARTWrite("=");
    UARTWriteHex(asuint(d));
    UARTWrite("\n");

    return 0;
}
