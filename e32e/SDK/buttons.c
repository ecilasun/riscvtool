#include "buttons.h"
#include <math.h> // for log2

volatile uint32_t *BUTTONCHANGE = (volatile uint32_t* )0x80001030;
volatile uint32_t *BUTTONCHANGEAVAIL = (volatile uint32_t* )0x80001034;
volatile uint32_t *BUTTONIMMEDIATESTATE = (volatile uint32_t* )0x80001038;

int IsKeyDown( enum EButtonNames _button )
{
    uint32_t mask = 1<<((uint32_t)_button);
    return ((*BUTTONIMMEDIATESTATE) & mask) ? 1:0;
}

const char *ButtonToString( enum EButtonNames _button )
{
    uint32_t buttonindex = log2(_button);
    switch (buttonindex)
    {
        case 0: return "center"; break;
        case 1: return "down"; break;
        case 2: return "left"; break;
        case 3: return "right"; break;
        case 4: return "up"; break;
        default: return "unknown"; break;
    }

    return "n/a";
}