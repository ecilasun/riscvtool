#include "usbc.h"
#include "basesystem.h"
#include "uart.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) DEVICE_USBC; // Receive fifo
volatile uint32_t *IO_USBCSTA = (volatile uint32_t* ) (DEVICE_USBC+4); // Output FIFO state

static uint32_t statusF = 0;
static uint32_t sparebyte = 0;

#define ASSERT_CS *IO_USBCTRX = 0x100;
#define RESET_CS *IO_USBCTRX = 0x101;

void USBFlushOutputFIFO()
{
    while ((*IO_USBCSTA)&0x1) {}
}

uint8_t USBGetGPX()
{
    return (*IO_USBCSTA)&0x2;
}

uint8_t USBReadByte(uint8_t command)
{
    ASSERT_CS
    *IO_USBCTRX = command;   // -> status in read FIFO
    statusF = *IO_USBCTRX;   // unused
    *IO_USBCTRX = 0;         // -> read value in read FIFO
    sparebyte = *IO_USBCTRX; // output value
    RESET_CS

    return sparebyte;
}

void USBWriteByte(uint8_t command, uint8_t data)
{
    ASSERT_CS
    *IO_USBCTRX = command | 0x02; // -> zero in read FIFO
    statusF = *IO_USBCTRX;        // unused
    *IO_USBCTRX = data;           // -> zero in read FIFO
    sparebyte = *IO_USBCTRX;      // unused
    RESET_CS
}

int USBReadBytes(uint8_t command, uint8_t length, uint8_t *buffer)
{
    ASSERT_CS
    *IO_USBCTRX = command;   // -> status in read FIFO
    statusF = *IO_USBCTRX;   // unused
    //*IO_USBCTRX = 0;         // -> read value in read FIFO
    //sparebyte = *IO_USBCTRX; // output value

    for (int i=0; i<length; i++)
    {
        *IO_USBCTRX = 0;          // send one dummy byte per input desired
        buffer[i] = *IO_USBCTRX;  // store data byte
    }
    RESET_CS

    return 0;
}

void USBWriteBytes(uint8_t command, uint8_t length, uint8_t *buffer)
{
    ASSERT_CS
    *IO_USBCTRX = command | 0x02;   // -> status in read FIFO
    statusF = *IO_USBCTRX;          // unused
    //*IO_USBCTRX = 0;         // -> read value in read FIFO
    //sparebyte = *IO_USBCTRX; // output value

    for (int i=0; i<length; i++)
    {
        *IO_USBCTRX = buffer[i];  // send one dummy byte per input desired
        sparebyte = *IO_USBCTRX;  // unused
    }
    RESET_CS
}

void USBCtlReset()
{
    // Reset
    USBWriteByte(rUSBCTL, bmCHIPRES);    // reset the MAX3420E 
    USBWriteByte(rUSBCTL, 0);            // remove the reset

    USBWriteByte(rGPIO, 0x0);            // No GPIO output

    USBFlushOutputFIFO();
    E32Sleep(3*ONE_MILLISECOND_IN_TICKS);

    // Wait for oscillator OK interrupt
    UARTWrite("Waiting for 12MHz oscillator...");
    uint8_t rd = 0;
    while ((rd & bmOSCOKIRQ) == 0)
    {
        rd = USBReadByte(rUSBIRQ);
        UARTWriteHexByte(rd);
        E32Sleep(3*ONE_MILLISECOND_IN_TICKS);
    }
    USBWriteByte(rUSBIRQ, bmOSCOKIRQ);
    UARTWrite("OK\n");

    if (rd&bmBUSACTIRQ)
        UARTWrite("usb bus active\n");
}

void USBInit()
{
    USBWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF); // MAX3420: SPI=full-duplex

    USBCtlReset();

    USBWriteByte(rUSBCTL, bmCONNECT | bmVBGATE);

    // Enable IRQs
    USBWriteByte(rEPIEN, bmSUDAVIE);
    USBWriteByte(rUSBIEN, bmURESDNIE | bmURESIE | bmSUSPIE);

    // Enable interrupt generation via INT pin
    USBWriteByte(rCPUCTL, bmIE);
}
