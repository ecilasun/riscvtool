#include "core.h"
#include "gpu.h"

#define CONSOLE_COLUMNS 40
#define CONSOLE_ROWS 30

void InitConsole();
void ClearConsole();
void ScrollConsole();
void SetConsoleCursor(const int x, const int y);
void EchoConsole(const char *echostring);
void EchoConsoleDecimal(const int32_t i);
void EchoConsoleHex(const int32_t i);
void EchoConsoleHexByte(const int32_t i);
void DrawConsole(struct EVideoContext *_context);
void GetConsoleCursor(int *x, int *y);
void ClearConsoleRow();
void ConsoleCursorStepBack();
void ConsoleStringAtRow(char *target);
