#include <stdio.h>
#include <cstdlib>

#include "core.h"
#include "audio.h"

int main()
{
    do{
        for (uint32_t i=0;i<65536;++i)
            *IO_AUDIOOUT = i;
    } while(1);

    return 0;
}
