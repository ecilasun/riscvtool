#include "basesystem.h"
#include "xadc.h"
#include <stdio.h>

int main()
{
    printf("ADC test\n");

	while (1)
    {
        unsigned int ch0 = ANALOGINPUTS[0];

        printf("0x%.8x\n", ch0);
    }

    return 0;
}
