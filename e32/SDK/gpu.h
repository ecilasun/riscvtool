#include <inttypes.h>

// Base address of frame buffer A
extern volatile uint8_t *GPUFB0;

// Base address of frame buffer B
//extern volatile uint8_t *GPUFB1;

// Base address of color palette, indexed from 0x00 to 0xFF
extern volatile uint32_t *GPUPAL_32;

// [31:18]: unused, TBD
// [17:17]: scanout page
// [16:16]: write page
// [15:0]: lane mask
extern volatile uint32_t *GPUCTL;

#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))
