#include <stdlib.h>
#include "core.h"
#include "console.h"
#include "gpu.h"

static char *consoleText;
static int cursorx = 0;
static int cursory = 0;

void InitConsole()
{
	consoleText = (char*)malloc(CONSOLE_COLUMNS*CONSOLE_ROWS*sizeof(char));
}

void ClearConsole()
{
    cursorx = 0;
    cursory = 0;
    for (int cy=0;cy<CONSOLE_ROWS;++cy)
        for (int cx=0;cx<CONSOLE_COLUMNS;++cx)
            consoleText[cx+cy*CONSOLE_COLUMNS] = ' ';
}

void ClearConsoleRow()
{
    // Clear the last row
    for (int cx=0;cx<CONSOLE_COLUMNS;++cx)
        consoleText[cx+cursory*CONSOLE_COLUMNS] = ' ';
}

void SetConsoleCursor(const int x, const int y)
{
    cursorx = x;
    cursory = y;

    cursorx = cursorx<0 ? 0:cursorx;
    cursorx = cursorx>CONSOLE_COLUMNS-1 ? CONSOLE_COLUMNS-1:cursorx;
    cursory = cursory<0 ? 0:cursory;
    cursory = cursory>CONSOLE_ROWS-1 ? CONSOLE_ROWS-1:cursory;
}

void ScrollConsole()
{
    for (int cy=0;cy<CONSOLE_ROWS-1;++cy)
        for (int cx=0;cx<CONSOLE_COLUMNS;++cx)
            consoleText[cx+cy*CONSOLE_COLUMNS] = consoleText[cx+(cy+1)*CONSOLE_COLUMNS];
    SetConsoleCursor(cursorx, CONSOLE_ROWS-1);
    ClearConsoleRow();
}

void GetConsoleCursor(int *x, int *y)
{
    *x = cursorx;
    *y = cursory;
}

void ConsoleCursorStepBack()
{
    --cursorx;
    if (cursorx<0)
    {
        cursorx = CONSOLE_COLUMNS-1;
        --cursory;
    }
    if (cursory<0)
    {
        cursorx = 0;
        cursory = 0;
    }
}

void EchoConsole(const char *echostring)
{
    char *str = (char*)echostring;
    while (*str != 0)
    {
        if (*str == '\r')
        {
            cursorx = 0;
        }
        else if (*str == '\n')
        {
			++cursory;
        }
        else
        {
            consoleText[cursorx+cursory*CONSOLE_COLUMNS] = *str;
            ++cursorx;
        }

        if (cursorx>CONSOLE_COLUMNS-1)
        {
            cursorx=0;
            cursory++;
        }
        if (cursory>CONSOLE_ROWS-1)
        {
            cursory=CONSOLE_ROWS-1;
            cursorx=0;
            ScrollConsole();
        }
        ++str;
    }
}

void EchoConsoleDecimal(const int32_t i)
{
    const char digits[] = "0123456789";
    char msg[] = "                   ";

    int d = 1000000000;
    uint32_t enableappend = 0;
    uint32_t m = 0;
    if (i<0)
        msg[m++] = '-';
    for (int c=0;c<10;++c)
    {
        uint32_t r = abs(i/d)%10;
        // Ignore preceeding zeros
        if ((r!=0) || enableappend || d==1)
        {
            enableappend = 1; // Rest of the digits can be appended
            msg[m++] = digits[r];
        }
        d = d/10;
    }
    msg[m] = 0;

    EchoConsole(msg);
}

void EchoConsoleHex(const int32_t i)
{
    const char hexdigits[] = "0123456789ABCDEF";
    char msg[] = "        ";
    msg[0] = hexdigits[((i>>28)%16)];
    msg[1] = hexdigits[((i>>24)%16)];
    msg[2] = hexdigits[((i>>20)%16)];
    msg[3] = hexdigits[((i>>16)%16)];
    msg[4] = hexdigits[((i>>12)%16)];
    msg[5] = hexdigits[((i>>8)%16)];
    msg[6] = hexdigits[((i>>4)%16)];
    msg[7] = hexdigits[(i%16)];
    EchoConsole(msg);
}

void EchoConsoleHexByte(const int32_t i)
{
    const char hexdigits[] = "0123456789ABCDEF";
    char msg[] = "  ";
    msg[0] = hexdigits[((i>>4)%16)];
    msg[1] = hexdigits[(i%16)];
    EchoConsole(msg);
}

void DrawConsole(struct EVideoContext *_context)
{
    for (int cy=0;cy<CONSOLE_ROWS;++cy)
        GPUPrintString(_context, 0, cy*8, &consoleText[cy*CONSOLE_COLUMNS], CONSOLE_COLUMNS);
}

void ConsoleStringAtRow(char *target)
{
    // NOTE: Input string must be >CONSOLE_COLUMNS bytes long to accomodate the null terminator
    int cx;
    for (cx=0; cx<cursorx; ++cx)
        target[cx] = consoleText[cursory*CONSOLE_COLUMNS+cx];
    target[cx] = 0;
}
