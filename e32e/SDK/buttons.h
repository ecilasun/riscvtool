#include <inttypes.h>

// Button state change FIFO output
extern volatile uint32_t *BUTTONCHANGE;
// Nonzero if there is data waiting at button state change FIFO
extern volatile uint32_t *BUTTONCHANGEAVAIL;
// Bypass the fifo and read current button states directly
// NOTE: This won't drain the FIFO, which still needs to happen to avoid filling it up (512 entries)
extern volatile uint32_t *BUTTONIMMEDIATESTATE;

// Bit masks for individual buttons
#define BUTTONMASK_CENTER 0x00000001
#define BUTTONMASK_DOWN 0x00000002
#define BUTTONMASK_LEFT 0x00000004
#define BUTTONMASK_RIGHT 0x00000008
#define BUTTONMASK_UP 0x00000010

enum EButtonNames {
    ButtonCenter,
    ButtonDown,
    ButtonLeft,
    ButtonRight,
    ButtonUp
};

int IsKeyDown( enum EButtonNames _button );
const char *ButtonToString( enum EButtonNames _button );
