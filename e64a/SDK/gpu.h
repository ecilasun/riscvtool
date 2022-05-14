#include <inttypes.h>

// Base addresses of frame buffers
extern volatile uint8_t *GPUFB;
extern volatile uint32_t *GPUFBWORD;
// Base address of color palette, word addressed
extern volatile uint32_t *GPUPAL_32;
// GPU control port
extern volatile uint32_t *GPUCTL;

// Helper macros
#define MAKERGBPALETTECOLOR(_r, _g, _b) (((_g&0xFF)<<16) | ((_r&0xFF)<<8) | (_b&0xFF))

#define FRAME_WIDTH 320
#define FRAME_WIDTH_IN_WORDS 80
#define FRAME_HEIGHT 240

void ClearScreen(uint32_t bgcolor);
uint32_t DrawText(const int col, const int row, const char *message);
void DrawTextLen(const int col, const int row, const int length, const char *message);
uint32_t DrawDecimal(const int col, const int row, const int32_t i);
