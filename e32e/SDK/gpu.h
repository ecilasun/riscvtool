#include <inttypes.h>

#define FRAME_WIDTH 320
#define FRAME_HEIGHT 240

#define GPUCMD_SETVPAGE    0x00000000
#define GPUCMD_SETPAL      0x00000001
#define GPUCMD_SETVMODE    0x00000002

#define MAKECOLORRGB24(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))
#define MAKEVMODEINFO(_mode, _scanEnable) ((_mode&0xF)<<4) | (_scanEnable&0x1)

uint8_t *GPUAllocateBuffer(const uint32_t _size);
void GPUSetVMode(const uint32_t _vmodeInfo);
void GPUSetVPage(const uint32_t _scanOutAddress64ByteAligned);
void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _rgba24);
void GPUPrintString(uint8_t *_vramBase, const int _col, const int _row, const char *_message, int _length);
void GPUClearScreen(uint8_t *_vramBase, const uint32_t _colorWord);
uint32_t GPUReadVBlankCounter();
