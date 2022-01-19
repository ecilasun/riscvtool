#include "buttons.h"

volatile uint32_t *BUTTONCHANGE = (volatile uint32_t* )0x20004000;
volatile uint32_t *BUTTONCHANGEAVAIL = (volatile uint32_t* )0x20004004;
volatile uint32_t *BUTTONIMMEDIATESTATE = (volatile uint32_t* )0x20004008;

int IsKeyDown( EButtonBames _button )
{
    uint32_t mask = 1<<((uint32_t)_button);
    return ((*BUTTONIMMEDIATESTATE) & mask) ? 1:0;
}
