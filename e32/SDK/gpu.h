#include <inttypes.h>

// Base addresses of frame buffers
extern volatile uint8_t *GPUFB0;
extern volatile uint8_t *GPUFB1;
// Base address of color palette, word addressed
extern volatile uint32_t *GPUPAL_32;
// GPU control/command port
extern volatile uint32_t *GPUCTL;

// Helper macros
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))
