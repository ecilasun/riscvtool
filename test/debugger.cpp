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

void int2architectureorderedstring(const uint32_t val, char *regstring)
{
    regstring[6] = hexdigits[((val>>28)%16)];
    regstring[7] = hexdigits[((val>>24)%16)];
    regstring[4] = hexdigits[((val>>20)%16)];
    regstring[5] = hexdigits[((val>>16)%16)];
    regstring[2] = hexdigits[((val>>12)%16)];
    regstring[3] = hexdigits[((val>>8)%16)];
    regstring[0] = hexdigits[((val>>4)%16)];
    regstring[1] = hexdigits[(val%16)];
    regstring[8] = 0;
}

void SendDebugPacketRegisters(cpu_context &task)
{
    char packetString[512];

    // All registers first
    for(uint32_t i=0;i<32;++i)
        int2architectureorderedstring(task.reg[i], &packetString[i*8]);

    // PC is sent last
    int2architectureorderedstring(task.PC, &packetString[32*8]);

    // Not sure if float registers follow?

    char checksumstr[3];
    strchecksum(packetString, checksumstr);

    EchoUART("+$");
    EchoUART(packetString);
    EchoUART("#");
    EchoUART(checksumstr);
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

void RemoveBreakPoint(cpu_context &task, uint32_t breakaddress)
{
    for (uint32_t i=0; i<task.num_breakpoints; ++i)
    {
        if (task.breakpoints[i].address == breakaddress)
        {
            // Restore saved instruction
            *(uint32_t*)(breakaddress) = task.breakpoints[i].originalinstruction;
            
            if (task.num_breakpoints-1 != i)
            {
                task.breakpoints[i].originalinstruction = task.breakpoints[task.num_breakpoints-1].originalinstruction;
                task.breakpoints[i].address = task.breakpoints[task.num_breakpoints-1].address;
            }

            task.num_breakpoints--;
            EchoConsole("RMV ");
            EchoConsoleHex(breakaddress);
            EchoConsole("\r\n");
        }
    }
}

void AddBreakPoint(cpu_context &task, uint32_t breakaddress)
{
    task.breakpoints[task.num_breakpoints].address = breakaddress;
    // Save instruction
    task.breakpoints[task.num_breakpoints].originalinstruction = *(uint32_t*)(breakaddress);
    task.num_breakpoints++;

    // Replace with EBREAK
    *(uint32_t*)(breakaddress) = 0x00100073;

    EchoConsole("BRK ");
    EchoConsoleHex(breakaddress);
    EchoConsole("\r\n");
}

uint32_t gdb_handler(cpu_context tasks[], uint32_t in_breakpoint)
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

    int has_break = 0;
    while (*IO_UARTRXByteCount)
    {
        char checkchar = *IO_UARTRX;

        if (checkchar == '\003')
            has_break = 1;
        processgdbpacket(checkchar);
    }

    if (has_break)
    {
        tasks[1].ctrlc = 1;
        SendDebugPacket("T02");
        return 0x1;
    }

    // ACK
    if (packetcomplete)
    {
        packetcomplete = 0;

        // qSupported: respond with 'hwbreak; swbreak'

        if (startswith(packetbuffer, "qSupported", 10))
            SendDebugPacket("swbreak+;hwbreak+;multiprocess-;PacketSize=1024");
        else if (startswith(packetbuffer, "vMustReplyEmpty", 15))
            SendDebugPacket(""); // Have to reply empty
        else if (startswith(packetbuffer, "Hg0", 3)) // Future ops apply to thread 0
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "qTStatus", 8))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "?", 1))
            SendDebugPacket("S05"); // SIGTRAP==5 https://man7.org/linux/man-pages/man7/signal.7.html
        else if (startswith(packetbuffer, "qfThreadInfo", 12))
            SendDebugPacket("l"); // Not supported
        else if (startswith(packetbuffer, "qSymbol", 7))
            SendDebugPacket("OK"); // No symtable info required
        else if (startswith(packetbuffer, "qAttached", 9))
            SendDebugPacket("1");
        else if (startswith(packetbuffer, "qOffsets", 8))
            SendDebugPacket("Text=0;Data=0;Bss=0"); // No relocation
        else if (startswith(packetbuffer, "vCont?", 6))
            SendDebugPacket("vCont;c;s;t"); // Continue/step/stop actions supported
        else if (startswith(packetbuffer, "vCont", 5))
        {
            if (packetbuffer[5]=='c') // Continue action
                EchoConsole("cont\r\n");
            if (packetbuffer[5]=='s') // Step action
                EchoConsole("step\r\n");
            if (packetbuffer[5]=='t') // Stop action
                EchoConsole("stop\r\n");
            SendDebugPacket(""); // Not sure what this is
        }
        else if (startswith(packetbuffer, "qL", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "Hc", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "qC", 2))
            SendDebugPacket(""); // Not supported
        else if (startswith(packetbuffer, "z0,", 3)) // remove breakpoint
        {
            uint32_t breakaddress;

            char hexbuf[12];
            int a=0,p=3;
            while (packetbuffer[p]!=',')
                hexbuf[a++] = packetbuffer[p++];
            hexbuf[a]=0;
            breakaddress = hex2int(hexbuf);

            RemoveBreakPoint(tasks[1], breakaddress);
            SendDebugPacket("OK");
        }
        else if (startswith(packetbuffer, "Z0,", 3)) // insert breakpoint
        {
            uint32_t breakaddress;

            char hexbuf[12];
            int a=0,p=3;
            while (packetbuffer[p]!=',')
                hexbuf[a++] = packetbuffer[p++];
            hexbuf[a]=0;
            breakaddress = hex2int(hexbuf); // KIND code is probably '4' after this (standard 32bit break)

            AddBreakPoint(tasks[1], breakaddress);
            SendDebugPacket("OK");
        }
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

            int2architectureorderedstring(tasks[1].reg[r], regstring);

            SendDebugPacket(regstring); // Return register data
        }
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

            int2architectureorderedstring(memval, regstring);

            SendDebugPacket(regstring);
        }
        else if (startswith(packetbuffer, "s", 1)) // Step
        {
            // TODO:

            SendDebugPacket("OK");
        }
        else if (startswith(packetbuffer, "c", 1)) // Continue
        {
            tasks[1].ctrlc = 8;
            SendDebugPacket("OK");
        }
        else // Unknown command
        {
            EchoConsole(">");
            EchoConsole(packetbuffer); // NOTE: Don't do this
            EchoConsole("\r\n");
        }
    }

    return 0x0; // TODO: might want to pass data back
}
