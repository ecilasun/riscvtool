#include <stdlib.h>
#include "core.h"
#include "gpu.h"

static char consoleText[32*24];
static int cursorx = 0;
static int cursory = 0;

void ClearConsole()
{
    cursorx = 0;
    cursory = 0;
    for (int cy=0;cy<24;++cy)
        for (int cx=0;cx<32;++cx)
            consoleText[cx+cy*32] = ' ';
}

void ClearConsoleRow()
{
    // Clear the last row
    for (int cx=0;cx<32;++cx)
        consoleText[cx+cursory*32] = ' ';
}

void SetConsoleCursor(const int x, const int y)
{
    cursorx = x;
    cursory = y;

    cursorx = cursorx<0 ? 0:cursorx;
    cursorx = cursorx>31 ? 31:cursorx;
    cursory = cursory<0 ? 0:cursory;
    cursory = cursory>23 ? 23:cursory;
}

void ScrollConsole()
{
    for (int cy=0;cy<23;++cy)
        for (int cx=0;cx<32;++cx)
            consoleText[cx+cy*32] = consoleText[cx+(cy+1)*32];
    SetConsoleCursor(cursorx, 23);
    ClearConsoleRow();
}

void GetConsoleCursor(int &x, int &y)
{
    x = cursorx;
    y = cursory;
}

void ConsoleCursorStepBack()
{
    --cursorx;
    if (cursorx<0)
    {
        cursorx = 31;
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
        if (*str == '\r' || *str == '\n')
        {
            if (*str == '\r') cursorx = 0;
            if (*str == '\n') ++cursory;
        }
        else
        {
            consoleText[cursorx+cursory*32] = *str;
            ++cursorx;
        }

        if (cursorx>31)
        {
            cursorx=0;
            cursory++;
        }
        if (cursory>23)
        {
            cursory=23;
            cursorx=0;
            ScrollConsole();
        }
        ++str;
    }
}

void EchoConsole(const int32_t i)
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

void DrawConsole()
{
    for (int cy=0;cy<24;++cy)
        PrintNDMA(0, 8*cy, 32, &consoleText[cy*32], true);
}

void ConsoleStringAtRow(char *target)
{
    // NOTE: Input string must be >32 bytes long to accomodate the null terminator
    int cx;
    for (cx=0; cx<cursorx; ++cx)
        target[cx] = consoleText[cursory*32+cx];
    target[cx] = 0;
}
