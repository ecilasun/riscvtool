#include "usbc.h"
#include "basesystem.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBCSTA = (volatile uint32_t* ) 0x80007004; // Output FIFO state

uint8_t USBWriteByte(uint8_t command, uint8_t data)
{
    command |= 0x02; // set write bit

    // Hardware sets CSn low for two bytes and brings it back high
    *IO_USBCTRX = command;
    *IO_USBCTRX = data;

    //waitForTxComplete();
    while (*IO_USBCSTA!=0) {}
    E32Sleep(1);

    volatile uint32_t readresA = *IO_USBCTRX; // status byte - R
    volatile uint32_t readresB = *IO_USBCTRX; // discard dummy byte

    return readresA;
}

uint8_t USBReadByte(uint8_t command)
{
    // Hardware sets CSn low for two bytes and brings it back high
    *IO_USBCTRX = command;
    *IO_USBCTRX = 0;

    //waitForTxComplete();
    while (*IO_USBCSTA!=0) {}
    E32Sleep(1);

    volatile uint32_t readresA = *IO_USBCTRX; // status unused
    volatile uint32_t readresB = *IO_USBCTRX; // return value

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
