#include "usbc.h"
#include "basesystem.h"
#include "uart.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBCSTA = (volatile uint32_t* ) 0x80007004; // Output FIFO state

static uint32_t statusF = 0;
static uint32_t sparebyte = 0;

#define ASSERT_CS *IO_USBCTRX = 0x100;
#define RESET_CS *IO_USBCTRX = 0x101;

/*
MAX3420E
For an SPI write cycle, any bytes following the
command byte are clocked into the MAX3420E on MOSI,
and zeros are clocked out on MISO.
For an SPI read cycle, any bytes following the command
byte are clocked out of the MAX3420E on MISO and the data
on MOSI is ignored.
At the conclusion of the SPI cycle (SS = 1), the MISO
outputs high impedance.*/

void USBFlushOutputFIFO()
{
    while (*IO_USBCSTA!=0) {}
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

void USBInit()
{
    // Half duplex without this
    UARTWrite("MAX3420e: full duplex mode\n");
    USBWriteByte(rPINCTL, bmFDUPSPI); // MAX3420: SPI=full-duplex 

    UARTWrite("MAX3420e: resetting...\n");
    USBWriteByte(rUSBCTL, bmCHIPRES); // reset the MAX3420E 
    USBWriteByte(rUSBCTL, 0); // remove the reset

    // Wait for oscillator OK interrupt
    while ((USBReadByte(rUSBIRQ) & bmOSCOKIRQ) == 0)
    {
        E32Sleep(3);
    }
    USBWriteByte(rUSBIRQ, bmOSCOKIRQ);
    UARTWrite("MAX3420e: oscillator OK\n");

    USBWriteByte(rPINCTL, bmINTLEVEL | gpxSOF);
    USBWriteByte(rGPIO, 0x0);

    USBWriteByte(rCPUCTL, bmIE);
    USBWriteByte(rUSBIEN, bmURESDNIE);
    USBWriteByte(rUSBCTL, bmVBGATE | bmCONNECT | bmHOSCSTEN);
    UARTWrite("MAX3420e: interrupts enabled\n");
}
