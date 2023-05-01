#include "usbc.h"
#include "basesystem.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBCCSn = (volatile uint32_t* ) 0x80007004; // CSn wire

uint8_t USBWriteByte(uint8_t command, uint8_t data)
{
    uint32_t readresA = 0;
    uint32_t readresB = 0;

    command |= 0x02;                         // set write bit

    //UARTWrite("W");
    *IO_USBCCSn = 0x0;
    //MAX_ASSERT_CS;

    //MAX_USART.STATUS = USART_TXCIF_bm;     // clear TX-complete flag
    //UARTWrite("C");
    *IO_USBCTRX = command;                   // send command byte
    //UARTWrite("D");
    *IO_USBCTRX = data;                      // send data

    //waitForTxComplete();
    E32Sleep(1);

    //MAX_RELEASE_CS;
    *IO_USBCCSn = 0x1;

    //UARTWrite("e");

    //UARTWrite("H:");
    uint32_t cntr = 0;
    uint32_t R = 0xFF;
    while(R==0xFF && (cntr++<255)) { R=*IO_USBCTRX; }

    readresA = R;           // status byte
    readresB = *IO_USBCTRX; // discard dummy byte

    //UARTWriteHexByte(readresA);
    //UARTWrite("\n");

    return readresA;
}

uint8_t USBReadByte(uint8_t command)
{
    volatile uint32_t readresA = 0;
    volatile uint32_t readresB = 0;

    //MAX_ASSERT_CS;
    //UARTWrite("R:");
    *IO_USBCCSn = 0x0;
    //MAX_USART.DATA = command;                   // send command byte
    //MAX_USART.DATA = 0;                         // dummy value for clocking in received data
    *IO_USBCTRX = command;
    *IO_USBCTRX = 0;

    /*UARTWrite("H:");
    uint32_t cntr = 0;
    while(*IO_USBCTRX==0xFF && (cntr++<255)) { }*/
    //waitForTxComplete();

    //MAX_RELEASE_CS;
    *IO_USBCCSn = 0x1;
    //statusF = MAX_USART.DATA;                   // update status byte

    readresA = *IO_USBCTRX; // status
    readresB = *IO_USBCTRX; // return value

    //UARTWriteHexByte(readresB);
    //UARTWrite("\n");

    return readresB;
}

void USBInit()
{
    USBWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF);
    USBWriteByte(rGPIO, 0x0);
    USBWriteByte(rUSBCTL, bmCONNECT | bmVBGATE);
    USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE);
    USBWriteByte(rCPUCTL, bmIE);
}

/*void maxPowerDown(void)
{
    uint8_t usbctrl = maxReadByte(rUSBCTL);
    usbctrl |= bmPWRDOWN; // set powerdown bit
    maxWriteByte(rUSBCTL, usbctrl);
}

void initMax3420(void)
{
    maxWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF);
    maxPowerDown(); // power down MAX3420
}

void initUsb(void)
{
    // reset MAX3420 and wait for oscillator stabilization
    maxWriteByte(rUSBCTL, bmCHIPRES);          // power up and reset MAX3420
    maxWriteByte(rUSBCTL, 0);                  // remove reset signal

    // wait for oscillator OK interrupt
    while ((maxReadByte(rUSBIRQ) & bmOSCOKIRQ) == 0)
    {
        E32Sleep(3);
    }
    maxWriteByte(rUSBIRQ, bmOSCOKIRQ);         // clear oscillator OK interrupt
    
    maxWriteByte(rCPUCTL, bmIE);
    maxWriteByte(rUSBIEN, bmURESIE | bmURESDNIE);
    maxWriteByte(rUSBCTL, bmVBGATE | bmCONNECT | bmHOSCSTEN);
}*/
