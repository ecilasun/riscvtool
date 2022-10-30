#include <inttypes.h>

// R/W port for GPU command access
extern volatile uint32_t *GPUFIFO;
extern volatile uint32_t *VRAM;
extern volatile uint8_t *VRAMBYTES;

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

#define GPUCMD_SETVPAGE    0x00000000
#define GPUCMD_SETPAL      0x00000001

#define MAKECOLORRGB24(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))

void GPUSetVPage(uint32_t _scanOutAddress64ByteAligned);
void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _rgba24);
