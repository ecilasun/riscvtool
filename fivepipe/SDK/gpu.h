#include <inttypes.h>

// R/W port for GPU command access
extern volatile uint32_t *GPUFIFO;
extern volatile uint32_t *VRAM;
extern volatile uint8_t *VRAMBYTES;

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240