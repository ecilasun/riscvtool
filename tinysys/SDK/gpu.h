#pragma once

#include <inttypes.h>

#define FRAME_WIDTH_MODE0 320
#define FRAME_HEIGHT_MODE0 240
#define FRAME_WIDTH_MODE1 640
#define FRAME_HEIGHT_MODE1 480

#define GPUCMD_SETVPAGE    0x00000000
#define GPUCMD_SETPAL      0x00000001
#define GPUCMD_SETVMODE    0x00000002

enum EVideoMode
{
    EVM_320_Pal,
    EVM_640_Pal,
    EVM_Count
};

enum EVideoScanoutEnable
{
    EVS_Disable,
    EVS_Enable,
    EVS_Count
};

struct EVideoContext
{
    enum EVideoMode m_mode;
    enum EVideoScanoutEnable m_scanEnable;
    uint32_t m_strideInWords;
    uint32_t m_scanoutAddressCacheAligned;
    uint32_t m_cpuWriteAddressCacheAligned;
};

// Utilities
uint8_t *GPUAllocateBuffer(const uint32_t _size);

// GPU side
void GPUSetDefaultPalette(struct EVideoContext *_context);
void GPUSetVMode(struct EVideoContext *_context, const enum EVideoMode _mode, const enum EVideoScanoutEnable _scanEnable);
void GPUSetScanoutAddress(struct EVideoContext *_context, const uint32_t _scanOutAddress64ByteAligned);
void GPUSetWriteAddress(struct EVideoContext *_context, const uint32_t _cpuWriteAddress64ByteAligned);
void GPUSetPal(const uint8_t _paletteIndex, const uint32_t _red, const uint32_t _green, const uint32_t _blue);
uint32_t GPUReadVBlankCounter();

// Software emulated
void GPUPrintString(struct EVideoContext *_context, const int _col, const int _row, const char *_message, int _length);
void GPUClearScreen(struct EVideoContext *_context, const uint32_t _colorWord);
