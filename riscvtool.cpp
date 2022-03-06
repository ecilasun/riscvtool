#if defined(CAT_LINUX)
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>

#include <chrono>
#include <thread>

#if defined(CAT_LINUX)
char devicename[512] = "/dev/ttyUSB1";
#else // CAT_WINDOWS
char devicename[512] = "\\\\.\\COM4";
#endif

unsigned int getfilelength(const fpos_t &endpos)
{
#if defined(CAT_LINUX)
	return (unsigned int)endpos.__pos;
#else // CAT_WINDOWS
    return (unsigned int)endpos;
#endif
}

class CSerialPort{
    public:

    CSerialPort() { }
    ~CSerialPort() { }

    bool Open()
    {
#if defined(CAT_LINUX)
        // Open COM port
        serial_port = open(devicename, O_RDWR);
        if (serial_port < 0 )
        {
            printf("Error %i from open: %s\n", errno, strerror(errno));
            return false;
        }

        struct termios tty;
        if(tcgetattr(serial_port, &tty) != 0)
        {
            printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
            return false;
        }

        // Set tty. flags
        tty.c_cflag &= ~PARENB; // No parity
        tty.c_cflag &= ~CSTOPB; // One stop bit
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8; // 8 bits
        tty.c_cflag &= ~CRTSCTS;
        tty.c_cflag |= CREAD | CLOCAL; // Not model (local), write

        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ISIG;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

        tty.c_oflag &= ~OPOST;
        tty.c_oflag &= ~ONLCR;
        //tty.c_oflag &= ~OXTABS;
        //tty.c_oflag &= ~ONOEOT;

        tty.c_cc[VTIME] = 50;
        tty.c_cc[VMIN] = 10;

        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200); // or only cfsetspeed(&tty, B115200);

        if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
            printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));

        printf("%s open\n", devicename);
        return true;

#else // CAT_WINDOWS
        hComm = CreateFileA(devicename, GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hComm != INVALID_HANDLE_VALUE)
        {
            serialParams.DCBlength = sizeof(serialParams);
            if (GetCommState(hComm, &serialParams))
            {
                serialParams.BaudRate = CBR_115200;
                serialParams.ByteSize = 8;
                serialParams.StopBits = ONESTOPBIT;
                serialParams.Parity = NOPARITY;

                if (SetCommState(hComm, &serialParams) != 0)
                {
                    timeouts.ReadIntervalTimeout = 50;
                    timeouts.ReadTotalTimeoutConstant = 50;
                    timeouts.ReadTotalTimeoutMultiplier = 10;
                    timeouts.WriteTotalTimeoutConstant = 50;
                    timeouts.WriteTotalTimeoutMultiplier = 10;
                    if (SetCommTimeouts(hComm, &timeouts) != 0)
                        return true;
                    else
                        printf("ERROR: can't set communication timeouts\n");
                    
                }
                else
                    printf("ERROR: can't set communication parameters\n");
            }
            else
                printf("ERROR: can't get communication parameters\n");
        }
        else
            printf("ERROR: can't open COM port\n");
        return false;
#endif
    }

    uint32_t Send(const char *_sendbytes, unsigned int _sendlength)
    {
#if defined(CAT_LINUX)
        int n = write(serial_port, _sendbytes, _sendlength);
        if (n < 0)
            printf("ERROR: write() failed\n");
        return _sendlength;
#else // CAT_WINDOWS
        DWORD byteswritten;
        // Send the command
        WriteFile(hComm, _sendbytes, _sendlength, &byteswritten, nullptr);
        return (uint32_t)byteswritten;
#endif
    }

    void Close()
    {
#if defined(CAT_LINUX)
        close(serial_port);
#else // CAT_WINDOWS
        CloseHandle(hComm);
#endif
    }

#if defined(CAT_LINUX)
    int serial_port{-1};
#else // CAT_WINDOWS
    HANDLE hComm{INVALID_HANDLE_VALUE};
    DCB serialParams{0};
    COMMTIMEOUTS timeouts{0};
#endif
};

#pragma pack(push,1)
struct SElfFileHeader32
{
    unsigned int m_Magic;       // Magic word (0x7F followed by 'ELF' -> 0x464C457F)
    unsigned char m_Class;
    unsigned char m_Data;
    unsigned char m_EI_Version;
    unsigned char m_OSABI;
    unsigned char m_ABIVersion;
    unsigned char m_Pad[7];
    unsigned short m_Type;      // NONE(0x00)/REL(0x01)/EXEC(0x02)/DYN(0x03)/CORE(0x04)/LOOS(0xFE00)/HIOS(0xFEFF)/LOPROC(0xFF00)/HIPROC(0xFFFF)
    unsigned short m_Machine;   // RISC-V: 0xF3
    unsigned int m_Version;
    unsigned int m_Entry;
    unsigned int m_PHOff;       // Program header offset in file
    unsigned int m_SHOff;
    unsigned int m_Flags;
    unsigned short m_EHSize;
    unsigned short m_PHEntSize;
    unsigned short m_PHNum;
    unsigned short m_SHEntSize;
    unsigned short m_SHNum;
    unsigned short m_SHStrndx;
};
struct SElfProgramHeader32
{
    unsigned int m_Type;
    unsigned int m_Offset;      // Offset of entry point
    unsigned int m_VAddr;
    unsigned int m_PAddr;
    unsigned int m_FileSz;
    unsigned int m_MemSz;       // Length of code in memory (includes code and data sections)
    unsigned int m_Flags;
    unsigned int m_Align;
};
struct SElfSectionHeader32
{
    unsigned int m_NameOffset;
    unsigned int m_Type;
    unsigned int m_Flags;
    unsigned int m_Addr;
    unsigned int m_Offset;
    unsigned int m_Size;
    unsigned int m_Link;
    unsigned int m_Info;
    unsigned int m_AddrAlign;
    unsigned int m_EntSize;
};
#pragma pack(pop)

void parseelfheader(unsigned char *_elfbinary, unsigned int groupsize)
{
    // Parse the header and check the magic word
    SElfFileHeader32 *fheader = (SElfFileHeader32 *)_elfbinary;

    if (fheader->m_Magic != 0x464C457F)
    {
        printf("Unknown magic, expecting 0x7F followed by 'ELF', got '%c%c%c%c' (%.8X)\n", fheader->m_Magic&0x000000FF, (fheader->m_Magic&0x0000FF00)>>8, (fheader->m_Magic&0x00FF0000)>>16, (fheader->m_Magic&0xFF000000)>>24, fheader->m_Magic);
        return;
    }

    // Parse the header and dump the executable
    SElfProgramHeader32 *pheader = (SElfProgramHeader32 *)(_elfbinary+fheader->m_PHOff);

    printf("memory_initialization_radix=16;\nmemory_initialization_vector=\n");
    //uint32_t sz = pheader->m_MemSz < pheader->m_FileSz ? pheader->m_MemSz : pheader->m_FileSz;
    if (groupsize == 4) // 32bit groups (4 bytes)
    {
        unsigned char *litteendian = (unsigned char *)(_elfbinary+pheader->m_Offset);
        for (unsigned int i=0; i<pheader->m_FileSz; ++i)
        {
            if (i!=0 && ((i%16) == 0))
                printf("\n");
            printf("%.2X", litteendian[(i&0xFFFFFFFC) + 3-(i%4)]);
            if (((i+1)%4)==0 && i!=pheader->m_FileSz-1)
                printf(" "); // 32bit separator
        }
    }
    else if (groupsize == 16) // 128bit groups (16 bytes)
    {
        unsigned int *litteendian = (unsigned int *)(_elfbinary+pheader->m_Offset);
        for (unsigned int i=0;i<pheader->m_FileSz/4;++i)
        {
            if (i!=0 && ((i%4) == 0))
                printf("\n");
            printf("%.8X", litteendian[(i&0xFFFFFFFC) + 3-(i%4)]);
        }
        // NOTE: IT IS VERY IMPORTANT THAT EACH OUTPUT IS PADDED WITH TRAILING ZEROS TO AVOID MIS-INTERPRETATION OF INPUT DATA!
        unsigned int leftover = 8-((pheader->m_FileSz/4)%8);
        if (leftover!=8)
            for (unsigned int i=0;i<leftover;++i)
                printf("00000000");
    }
    else if (groupsize == 32) // 256bit groups (32 bytes)
    {
        unsigned int *litteendian = (unsigned int *)(_elfbinary+pheader->m_Offset);
        for (unsigned int i=0;i<pheader->m_FileSz/4;++i)
        {
            if (i!=0 && ((i%8) == 0))
                printf("\n");
            printf("%.8X", litteendian[(i&0xFFFFFFF8) + 7-(i%8)]);
        }
        // NOTE: IT IS VERY IMPORTANT THAT EACH OUTPUT IS PADDED WITH TRAILING ZEROS TO AVOID MIS-INTERPRETATION OF INPUT DATA!
        unsigned int leftover = 8-((pheader->m_FileSz/4)%8);
        if (leftover!=8)
            for (unsigned int i=0;i<leftover;++i)
                printf("00000000");
    }

    printf(";");
}

void dumpelf(char *_filename, unsigned int groupsize)
{
    FILE *fp;
    fp = fopen(_filename, "rb");
    if (!fp)
    {
        printf("ERROR: can't open ELF file %s\n", _filename);
        return;
    }
	unsigned int filebytesize = 0;
	fpos_t pos, endpos;
	fgetpos(fp, &pos);
	fseek(fp, 0, SEEK_END);
	fgetpos(fp, &endpos);
	fsetpos(fp, &pos);
    filebytesize = getfilelength(endpos);

    // TODO: Actual binary to send starts at 0x1000
    unsigned char *bytestosend = new unsigned char[filebytesize];
    fread(bytestosend, 1, filebytesize, fp);
    fclose(fp);

    parseelfheader(bytestosend, groupsize);

    delete [] bytestosend;
}

unsigned int generateelfuploadpackage(unsigned char *_elfbinaryread, unsigned char *_elfbinaryout)
{
    // Parse the header and check the magic word
    SElfFileHeader32 *fheader = (SElfFileHeader32 *)_elfbinaryread;

    if (fheader->m_Magic != 0x464C457F)
    {
        printf("ERROR: Unknown magic, expecting 0x7F followed by 'ELF', got '%c%c%c%c' (%.8X)\n", fheader->m_Magic&0x000000FF, (fheader->m_Magic&0x0000FF00)>>8, (fheader->m_Magic&0x00FF0000)>>16, (fheader->m_Magic&0xFF000000)>>24, fheader->m_Magic);
        return 0xFFFFFFFF;
    }

    // Parse the header and dump the executable
    SElfProgramHeader32 *pheader = (SElfProgramHeader32 *)(_elfbinaryread+fheader->m_PHOff);

    // Aliasing to re-use same buffer
    unsigned int *targetarea = (unsigned int*)_elfbinaryout;
    unsigned int *sourcearea = (unsigned int*)(_elfbinaryread+pheader->m_Offset);
    unsigned int binarysize = pheader->m_MemSz;

    printf("Preparing binary package\n");
    for (unsigned int i=0;i<binarysize/4;++i)
        targetarea[i] = sourcearea[i];

    return binarysize;
}

void sendelf(char *_filename, const unsigned int _target=0x00000000)
{
    FILE *fp;
    fp = fopen(_filename, "rb");
    if (!fp)
    {
        printf("ERROR: can't open ELF file %s\n", _filename);
        return;
    }

	unsigned int filebytesize = 0;
	fpos_t pos, endpos;
	fgetpos(fp, &pos);
	fseek(fp, 0, SEEK_END);
	fgetpos(fp, &endpos);
	fsetpos(fp, &pos);
    filebytesize = getfilelength(endpos);

    unsigned char *bytestoread = new unsigned char[filebytesize];
    unsigned char *bytestosend = new unsigned char[filebytesize];
    fread(bytestoread, 1, filebytesize, fp);
    fclose(fp);

    SElfFileHeader32 *fheader = (SElfFileHeader32 *)bytestoread;
    SElfProgramHeader32 *pheader = (SElfProgramHeader32 *)(bytestoread+fheader->m_PHOff);
    printf("Program VADDR=0x%.8X, PADDR 0x%.8X relocated to 0x%.8X\n", pheader->m_VAddr, pheader->m_PAddr, _target);
    unsigned int relativeStartAddress = (fheader->m_Entry-pheader->m_PAddr)+_target;
    printf("Executable entry point is at 0x%.8X (new relative entry point: 0x%.8X)\n", fheader->m_Entry, relativeStartAddress);
    unsigned int stringtableindex = fheader->m_SHStrndx;
    SElfSectionHeader32 *stringtablesection = (SElfSectionHeader32 *)(bytestoread+fheader->m_SHOff+fheader->m_SHEntSize*stringtableindex);
    char *names = (char*)(bytestoread+stringtablesection->m_Offset);

    CSerialPort serial;
    if (serial.Open() == false)
        return;

    printf("Sending ELF binary over COM4 @115200 bps\n");

    // Send all binary sections to their correct addresses
    for(int i=0; i<fheader->m_SHNum; ++i)
    {
        SElfSectionHeader32 *sheader = (SElfSectionHeader32 *)(bytestoread+fheader->m_SHOff+fheader->m_SHEntSize*i);

        char sectionname[128];
        int n=0;
        do
        {
            sectionname[n] = names[sheader->m_NameOffset+n];
            ++n;
        }
        while(names[sheader->m_NameOffset+n]!='.' && sheader->m_NameOffset+n<stringtablesection->m_Size);
        sectionname[n] = 0;

        if (!strcmp(sectionname, ".bss") || !strcmp(sectionname, ".stack"))
        {
            printf("SKIPPING: '%s' @0x%.8X len:%.8X off:%.8X\n", sectionname, (sheader->m_Addr-pheader->m_PAddr)+_target, sheader->m_Size, sheader->m_Offset);
            continue;
        }

        /*if (!strcmp(sectionname, ".got"))
        {
            // If we're relocating the ELF, we need to patch up the entries in: extern  Elf32_Addr  _GLOBAL_OFFSET_TABLE_[];
            uint32_t *got = (uint32_t*)(bytestoread+sheader->m_Addr);
            for (uint32_t i=0;i<sheader->m_Size/4;++i)
                printf("GOE: %d: %.8X\n", i, got[i]);
        }*/

        //if (sheader->m_Type == 0x1 || sheader->m_Type == 0xE || sheader->m_Type == 0xF || sheader->m_Type == 0x10) // Progbits/Iniarray/Finiarray/Preinitarray
        if (sheader->m_Flags & 0x00000007 && sheader->m_Size!=0) // writeable/alloc/exec and non-zero
        {
            printf("SENDING: '%s' @0x%.8X len:%.8X off:%.8X...\n", sectionname, (sheader->m_Addr-pheader->m_PAddr)+_target, sheader->m_Size, sheader->m_Offset);

            char commandtosend[512];
            int commandlength=2;
            sprintf(commandtosend, "B%c", 13);

            unsigned int blobheader[8]; // Space for the future
            ((unsigned int*)blobheader)[0] = (sheader->m_Addr-pheader->m_PAddr)+_target; // relative start address
            ((unsigned int*)blobheader)[1] = sheader->m_Size; // binary section size

            uint32_t byteswritten = 0;

            // Send the string "B\r"
            byteswritten += serial.Send(commandtosend, commandlength);
            // Wait a bit for the receiving end to start accepting
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Send 8 byte header
            byteswritten += serial.Send((char*)blobheader, 8);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Send data
            byteswritten += serial.Send((char*)(bytestoread+sheader->m_Offset), sheader->m_Size);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            if (byteswritten != 0)
                ;//printf("done (0x%.8X+0xC bytes written)\n", byteswritten-0xC);
            else
                printf("Failed!\n");
        }
    }

    // Start the executable
    {
        char commandtosend[512];
        int commandlength=2;
        sprintf(commandtosend, "R%c", 13);

        unsigned int blobheader[8]; // Space for the future
        ((unsigned int*)blobheader)[0] = relativeStartAddress; // relative start address

        printf("Branching to 0x%.8X\n", relativeStartAddress);

        uint32_t byteswritten = 0;

        // Send the string "R\r"
        byteswritten += serial.Send(commandtosend, commandlength);
        // Wait a bit for the receiving end to start accepting
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Send start address
        byteswritten += serial.Send((char*)blobheader, 4);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    serial.Close();

    delete [] bytestoread;
    delete [] bytestosend;
}

int main(int argc, char **argv)
{
    if (argc <= 4)
    {
        printf("RISCVTool\n");
        printf("Usage: riscvtool.exe binaryfilename [-sendelf hexaddress usbdevicename | -makerom groupbytesize]\n");
        printf("NOTE: Default device name is %s", devicename);
        return -1;
    }

    if (strstr(argv[2], "-makerom"))
    {
        unsigned int groupsize = (unsigned int)strtoul(argv[3], nullptr, 10);
        dumpelf(argv[1], groupsize);
    }
    if (strstr(argv[2], "-sendelf"))
    {
        unsigned int target = (unsigned int)strtoul(argv[3], nullptr, 16);
        if (argc > 4)
            strcpy(devicename, argv[4]);
        sendelf(argv[1], target);
    }
    return 0;
}
