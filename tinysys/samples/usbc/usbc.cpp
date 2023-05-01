#include "basesystem.h"
#include "uart.h"
#include "usbc.h"

int main(int argc, char *argv[])
{
    UARTWrite("USB-C test\n");

    if (argc>1)
    {
        UARTWrite("Bringing up USB-C\n");
        USBInit();
    }
    else
    {
        UARTWrite("USB-C SPI r/w diagnosis\n");

        // Half duplex without this
        USBWriteByte(rPINCTL, bmFDUPSPI); // MAX3420: SPI=full-duplex 

        USBWriteByte(rUSBCTL, bmCHIPRES); // reset the MAX3420E 
        USBWriteByte(rUSBCTL, 0); // remove the reset 

        // Wait for oscillator OK interrupt
        while ((USBReadByte(rUSBIRQ) & bmOSCOKIRQ) == 0)
        {
            E32Sleep(3);
        }
        USBWriteByte(rUSBIRQ, bmOSCOKIRQ);
        UARTWrite("Oscillator OK\n");

        UARTWrite("Testing IEN\nShould see: 01 02 04 08 10 20 40 80\n");
        uint8_t wr = 0x01; // initial register write value 
        for(int j=0; j<8; j++) 
        { 
            USBWriteByte(rUSBIEN, wr);

            uint8_t rd = USBReadByte(rUSBIEN);

            UARTWriteHex(rd);
            UARTWrite(" ");

            wr <<= 1; // Put a breakpoint here. Values of 'rd' should be 01,02,04,08,10,20,40,80 
        }
        UARTWrite("\n");
    }

    return 0;
}
