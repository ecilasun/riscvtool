#pragma once

#include <inttypes.h>

extern volatile unsigned char* VRAM;
extern volatile unsigned char* UARTRX;
extern volatile unsigned char* UARTTX;
extern volatile unsigned int* UARTRXStatus;
extern volatile unsigned int ROMResetVector;

void Print(const int ox, const int oy, const char *message);
void Print(const int ox, const int oy, const int maxlen, const char *message);
void PrintMasked(const int ox, const int oy, const char *message);
void PrintMasked(const int ox, const int oy, const int maxlen, const char *message);
void EchoUART(const char *_message);
void ClearScreen(const uint8_t color);
unsigned int Random();