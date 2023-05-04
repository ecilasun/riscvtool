#include "usbc.h"
#include "basesystem.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBCSTA = (volatile uint32_t* ) 0x80007004; // Output FIFO state

static uint32_t statusF = 0;
static uint32_t sparebyte = 0;

uint8_t USBReadByte(uint8_t command)
{
    *IO_USBCTRX = 0x100; // CS low
    *IO_USBCTRX = command;
    *IO_USBCTRX = 0;

    statusF = *IO_USBCTRX; // status unused
    sparebyte = *IO_USBCTRX;
    *IO_USBCTRX = 0x101; // CS high

    return sparebyte;
}

void USBWriteByte(uint8_t command, uint8_t data)
{
    *IO_USBCTRX = 0x100; // CS low
    *IO_USBCTRX = command | 0x02; // set write bit
    *IO_USBCTRX = data;

    statusF = *IO_USBCTRX; // status byte - R
    sparebyte = *IO_USBCTRX; // discard dummy byte
    *IO_USBCTRX = 0x101; // CS high
}

int USBReadBytes(uint8_t command, uint8_t length, uint8_t *buffer, uint8_t mask)
{
    uint8_t i;

    if (length == 0)
        return 0;

    *IO_USBCTRX = 0x100; // CS low
    *IO_USBCTRX = command;                   // send command byte
    while (*IO_USBCSTA!=0) {}                // wait until commands to fly across
    //E32Sleep(1);
    uint8_t statusF = *IO_USBCTRX;           // store status byte
    if ((statusF & mask) == 0)               // check mask, return error if status bits not set
    {
        *IO_USBCTRX = 0x101; // CS high
        return 1;
    }
    
    *IO_USBCTRX = 0;
    /* receive data */
    for (i=0; i<length-1; i++)
    {
        *IO_USBCTRX = 0;                     // send dummy byte
        buffer[i] = *IO_USBCTRX;             // store data byte
    }
    buffer[i] = *IO_USBCTRX;                 // store last data byte
    *IO_USBCTRX = 0x101; // CS high
 
    return 0;
}

void USBInit()
{
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
    //UARTWrite("Oscillator OK\n");

    USBWriteByte(rPINCTL, bmINTLEVEL | gpxSOF);
    USBWriteByte(rGPIO, 0x0);

    USBWriteByte(rCPUCTL, bmIE);
    USBWriteByte(rUSBIEN, bmURESDNIE);
    USBWriteByte(rUSBCTL, bmVBGATE | bmCONNECT | bmHOSCSTEN);
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
