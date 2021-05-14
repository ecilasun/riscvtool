#include "utils.h"

void ClearConsole();
void ScrollConsole();
void SetConsoleCursor(const int x, const int y);
void EchoConsole(const char *echostring);
void EchoConsole(const int32_t i);
void DrawConsole();
void GetConsoleCursor(int &x, int &y);
void ClearConsoleRow();
void ConsoleCursorStepBack();
