#include <inttypes.h>

// R/W port for LED status access
extern volatile uint32_t *LEDSTATE;

uint32_t GetLEDState();
void SetLEDState(const uint32_t state);
