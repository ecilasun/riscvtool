#include "utils.h"
#include "console.h"
#include <setjmp.h>
#include "debugger.h"

const char hexdigits[] = "0123456789ABCDEF";

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

    checksumstr[0] = hexdigits[((checksum>>4)%16)];
    checksumstr[1] = hexdigits[(checksum%16)];
    checksumstr[2] = 0;
}

void SendDebugPacketRegisters(cpu_context &task)
{
    char packetString[512];
    for(uint32_t i=0;i<32;++i)
    {
        packetString[i*8+6] = hexdigits[((task.reg[i]>>28)%16)];
        packetString[i*8+7] = hexdigits[((task.reg[i]>>24)%16)];
        packetString[i*8+4] = hexdigits[((task.reg[i]>>20)%16)];
        packetString[i*8+5] = hexdigits[((task.reg[i]>>16)%16)];
        packetString[i*8+2] = hexdigits[((task.reg[i]>>12)%16)];
        packetString[i*8+3] = hexdigits[((task.reg[i]>>8)%16)];
        packetString[i*8+0] = hexdigits[((task.reg[i]>>4)%16)];
        packetString[i*8+1] = hexdigits[(task.reg[i]%16)];
    }

    // PC last
    packetString[32*8+6] = hexdigits[((task.PC>>28)%16)];
    packetString[32*8+7] = hexdigits[((task.PC>>24)%16)];
    packetString[32*8+4] = hexdigits[((task.PC>>20)%16)];
    packetString[32*8+5] = hexdigits[((task.PC>>16)%16)];
    packetString[32*8+2] = hexdigits[((task.PC>>12)%16)];
    packetString[32*8+3] = hexdigits[((task.PC>>8)%16)];
    packetString[32*8+1] = hexdigits[((task.PC>>4)%16)];
    packetString[32*8+0] = hexdigits[(task.PC%16)];

    packetString[32*8+8] = 0;

    char checksumstr[3];
    strchecksum(packetString, checksumstr);

    EchoUART("+$");
    EchoUART(packetString);
    EchoUART("#");
    EchoUART(checksumstr);

    // EchoConsole("+$");
    // EchoConsole(packetString);
    // EchoConsole("#");
    // EchoConsole(checksumstr);
    // EchoConsole("\r\n");
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

uint32_t hex2int(char *hex)
{
    uint32_t val = 0;
    while (*hex) {
        uint8_t nibble = *hex++;
        if (nibble >= '0' && nibble <= '9')
            nibble = nibble - '0';
        else if (nibble >= 'a' && nibble <='f')
            nibble = nibble - 'a' + 10;
        else if (nibble >= 'A' && nibble <='F')
            nibble = nibble - 'A' + 10;
        val = (val << 4) | (nibble & 0xF);
    }
    return val;
}

uint32_t gdb_handler(cpu_context tasks[])
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

        //EchoConsole(">");
        //EchoConsole(packetbuffer); // NOTE: Don't do this
        //EchoConsole("\r\n");

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
        {
            SendDebugPacketRegisters(tasks[1]); // TODO: need to pick a task somehow
        }
        else if (startswith(packetbuffer, "p", 1)) // Print register, p??
        {
            char hexbuf[4], regstring[9];
            int r=0,p=1;
            while (packetbuffer[p]!='#')
                hexbuf[r++] = packetbuffer[p++];
            hexbuf[r]=0;
            r = hex2int(hexbuf);

            regstring[6] = hexdigits[((tasks[1].reg[r]>>28)%16)];
            regstring[7] = hexdigits[((tasks[1].reg[r]>>24)%16)];
            regstring[4] = hexdigits[((tasks[1].reg[r]>>20)%16)];
            regstring[5] = hexdigits[((tasks[1].reg[r]>>16)%16)];
            regstring[2] = hexdigits[((tasks[1].reg[r]>>12)%16)];
            regstring[3] = hexdigits[((tasks[1].reg[r]>>8)%16)];
            regstring[0] = hexdigits[((tasks[1].reg[r]>>4)%16)];
            regstring[1] = hexdigits[(tasks[1].reg[r]%16)];
            regstring[8] = 0;

            SendDebugPacket(regstring); // Return register data
        }
        else if (startswith(packetbuffer, "qSymbol", 7))
            SendDebugPacket("OK"); // No symtable info required
        else if (startswith(packetbuffer, "D", 1)) // Detach
            SendDebugPacket("");
        else if (startswith(packetbuffer, "m", 1)) // Read memory, maddr,count
        {
            char addrbuf[12], cntbuf[12];
            int a=0,c=0, p=1;
            while (packetbuffer[p]!=',')
                addrbuf[a++] = packetbuffer[p++];
            addrbuf[a]=0;
            while (packetbuffer[p]!='#')
                cntbuf[c++] = packetbuffer[p++];
            cntbuf[c]=0;
            a = hex2int(addrbuf);
            c = hex2int(cntbuf);

            char regstring[64];
            uint32_t memval = *(uint32_t*)a;
            regstring[6] = hexdigits[((memval>>28)%16)];
            regstring[7] = hexdigits[((memval>>24)%16)];
            regstring[4] = hexdigits[((memval>>20)%16)];
            regstring[5] = hexdigits[((memval>>16)%16)];
            regstring[2] = hexdigits[((memval>>12)%16)];
            regstring[3] = hexdigits[((memval>>8)%16)];
            regstring[0] = hexdigits[((memval>>4)%16)];
            regstring[1] = hexdigits[(memval%16)];
            regstring[8] = 0;

            SendDebugPacket(regstring);
        }
    }

    return 0x0; // TODO: might want to pass data back
}
