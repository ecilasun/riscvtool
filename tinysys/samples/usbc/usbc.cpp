#include "basesystem.h"
#include "uart.h"
#include "usbc.h"

int main()
{
    UARTWrite("USB-C test\n");

    USBInit();

    UARTWrite("Something should happen now...\n");

    /*while (1)
    {
        //...;
    }*/

    /*UARTWriteHexByte(USBCTxRx(rPINCTL));
    UARTWriteHexByte(USBCTxRx(bmFDUPSPI | bmINTLEVEL)); // Full duplex

    UARTWriteHexByte(USBCTxRx(rUSBCTL)); // Chip reset
    UARTWriteHexByte(USBCTxRx(bmCHIPRES));

    UARTWriteHexByte(USBCTxRx(rUSBCTL)); // Chip idle
    UARTWriteHexByte(USBCTxRx(0));

    USBCTxRx(rUSBCTL); // Connect
    USBCTxRx(bmCONNECT);

    uint32_t wr = 0x1;
    for (uint32_t i=0;i<8;++i)
    {
        UARTWriteHexByte(USBCTxRx(rUSBIEN));
        UARTWriteHexByte(USBCTxRx(wr));
        wr <<= 1;
    }*/

    return 0;
}
