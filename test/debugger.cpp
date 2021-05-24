#include "utils.h"
#include "console.h"
#include <setjmp.h>
#include "debugger.h"


void putDebugChar(int dbgchar)
{
    IO_UARTTX[0] = (uint8_t)dbgchar;
}

int getDebugChar()
{
    while (IO_UARTRXByteCount == 0) { }
    return IO_UARTRX[0];
}

void exceptionHandler()
{
    // NOTE: This will be embedded in the core exception handler
}

void flush_i_cache()
{
    // TODO
}

void exceptionHandler (int exception_number, void *exception_address)
{

}

// void *memset(void *, int, int) also in stdlib

// ----------------------------------------------------------

char packetbuffer[1200]; // Report 1024 but allocate more space just in case
int packetcursor = 0;
int checksumcounter = 0;
int packetcomplete = 0;

void processgdbpacket(char incoming)
{
    if (incoming == '\003')
    {
        EchoConsole("INTRQ\r\n");
    }

    if (checksumcounter != 0)
    {
        packetbuffer[packetcursor++] = incoming;
        checksumcounter--;
        if (checksumcounter == 0)
        {
            packetcomplete = 1;
            packetbuffer[packetcursor] = 0;
            packetcursor = 0;
        }
    }
    else
    {
        if (incoming == '$')
        {
            packetcursor = 0;
            packetcomplete = 0;
        }
        else if (incoming == '#')
        {
            packetbuffer[packetcursor++] = incoming;
            checksumcounter = 2;
        }
        else
            packetbuffer[packetcursor++] = incoming;
    }
}

int startswith(char *source, const char *substr, int len)
{
    int cmpcnt = 0;

    while ((substr[cmpcnt] != 0) && (source[cmpcnt] == substr[cmpcnt]))
        ++cmpcnt;
    
    return (cmpcnt == len) ? 1 : 0;
}

void strchecksum(const char *str, char *checksumstr)
{
    int checksum = 0;
    int i=0;
    while(str[i]!=0)
    {
        checksum += str[i];
        ++i;
    }
    checksum = checksum%256;

    const char hexdigits[] = "0123456789ABCDEF";
    checksumstr[0] = hexdigits[((checksum>>4)%16)];
    checksumstr[1] = hexdigits[(checksum%16)];
    checksumstr[2] = 0;
}

void SendDebugPacket(const char *packetString)
{
    char checksumstr[3];
    strchecksum(packetString, checksumstr);

    EchoUART("+$");
    EchoUART(packetString);
    EchoUART("#");
    EchoUART(checksumstr);

    // Debug dump
    /*EchoConsole("<");
    EchoConsole("+$");
    EchoConsole(packetString);
    EchoConsole("#");
    EchoConsole(checksumstr);
    EchoConsole("\r\n");*/
}

void gdb_stub()
{
    // NOTES:
    // Checksum is computed as the modulo 256 sum of the packet info characters.
    // For each packet received, responses is either + (for accepted) or - (for retry): $packet-data#checksum
    // Packets start with $, end with # and a two-digit hex checksum
    // GDB has a cache of registers

    // Some Commands:
    // g: request value of CPU registers
    // G: set value of CPU registers
    // maddr,count: read count bytes at addr
    // Maddr,count:... write count bytes at addr
    // c c [addr] resume execution at current PC or at addr if supplied
    // s s [addr] step the program one insruction from current PC or addr if supplied
    // k kill the target program
    // ? report the most recent signal
    // T allows remote to send only the registers required for quick decisions (step/conditional breakpoints)

    while (*IO_UARTRXByteCount)
    {
        char checkchar = *IO_UARTRX;

        processgdbpacket(checkchar);
        //EchoConsoleHexByte(checkchar);

        /*if (checkchar == 0x3) // CTRL+C
        {
            //have_a_break = 1; // Break the running program
        }*/
    }

    // ACK
    if (packetcomplete)
    {
        packetcomplete = 0;

        // qSupported: respond with 'hwbreak; swbreak'

        EchoConsole(">");
        EchoConsole(packetbuffer); // NOTE: Don't do this
        EchoConsole("\r\n");

        if (startswith(packetbuffer, "qSupported", 10))
            SendDebugPacket("swbreak+;hwbreak+;multiprocess-;PacketSize=1024");
        else if (startswith(packetbuffer, "vMustReplyEmpty", 15))
            SendDebugPacket(""); // Have to reply empty
        else if (startswith(packetbuffer, "Hg0", 3)) // Future ops apply to thread 0
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "qTStatus", 8))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "?", 1))
            SendDebugPacket("S05");
        else if (startswith(packetbuffer, "qfThreadInfo", 12))
            SendDebugPacket("l"); // Not supported
        else if (startswith(packetbuffer, "vCont?", 6))
            SendDebugPacket("OK"); // Continue
        else if (startswith(packetbuffer, "vCont", 5))
            SendDebugPacket(""); // Not sure what this is
        else if (startswith(packetbuffer, "qL", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "Hc", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "qC", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "qAttached", 9))
            SendDebugPacket("1");
        else if (startswith(packetbuffer, "qOffsets", 8))
            SendDebugPacket("Text=0;Data=0;Bss=0"); // No relocation
        else if (startswith(packetbuffer, "g", 1)) // List registers
            SendDebugPacket("0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"); // Return register data
        else if (startswith(packetbuffer, "p", 1)) // Print register, p??
            SendDebugPacket("00000000"); // Return register data
        else if (startswith(packetbuffer, "qSymbol", 7))
            SendDebugPacket("OK"); // No symtable info required
        else if (startswith(packetbuffer, "D", 1)) // Detach
            SendDebugPacket("OK");
        else if (startswith(packetbuffer, "m", 1)) // Read memory, maddr,count
        {
            SendDebugPacket("00000000");
        }
    }
}
