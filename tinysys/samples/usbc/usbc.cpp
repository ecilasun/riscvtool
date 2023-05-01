#include "basesystem.h"
#include "uart.h"
#include "usbc.h"

// A single SPI transaction is a write from master followed by a read from slave's output
/*uint8_t __attribute__ ((noinline)) USBCTxRx(const uint8_t outbyte)
{
   *IO_USBCTRX = outbyte;
   return *IO_USBCTRX;
}*/

 //#define SETBIT(reg,val) Max_wreg(reg,(Max_rreg(reg)|val));
 //#define CLRBIT(reg,val) Max_wreg(reg,(Max_rreg(reg)&~val));
 
// ******** END OF BUG FIX**********
 
 #define MSB(word) (BYTE)(((WORD)(word) >> 8) & 0xff)
 #define LSB(word) (BYTE)((WORD)(word) & 0xff)
 
// MAX3420E Registers
 #define rEP0FIFO    0<<3
 #define rEP1OUTFIFO 1<<3
 #define rEP2INFIFO  2<<3
 #define rEP3INFIFO  3<<3
 #define rSUDFIFO    4<<3
 #define rEP0BC      5<<3
 #define rEP1OUTBC   6<<3
 #define rEP2INBC    7<<3
 #define rEP3INBC    8<<3
 #define rEPSTALLS   9<<3
 #define rCLRTOGS    10<<3
 #define rEPIRQ      11<<3
 #define rEPIEN      12<<3
 #define rUSBIRQ     13<<3
 #define rUSBIEN     14<<3
 #define rUSBCTL     15<<3
 #define rCPUCTL     16<<3
 #define rPINCTL     17<<3
 #define rRevision   18<<3
 #define rFNADDR     19<<3
 #define rGPIO       20<<3
 
 
 
 
// MAX3420E bit masks for register bits
// R9 EPSTALLS Register
 #define bmACKSTAT   0x40
 #define bmSTLSTAT   0x20
 #define bmSTLEP3IN  0x10
 #define bmSTLEP2IN  0x08
 #define bmSTLEP1OUT 0x04
 #define bmSTLEP0OUT 0x02
 #define bmSTLEP0IN  0x01
 
// R10 CLRTOGS Register
 #define bmEP3DISAB  0x80
 #define bmEP2DISAB  0x40
 #define bmEP1DISAB  0x20
 #define bmCTGEP3IN  0x10
 #define bmCTGEP2IN  0x08
 #define bmCTGEP1OUT 0x04
 
// R11 EPIRQ register bits
 #define bmSUDAVIRQ  0x20
 #define bmIN3BAVIRQ 0x10
 #define bmIN2BAVIRQ 0x08
 #define bmOUT1DAVIRQ 0x04
 #define bmOUT0DAVIRQ 0x02
 #define bmIN0BAVIRQ 0x01
 
// R12 EPIEN register bits
 #define bmSUDAVIE   0x20
 #define bmIN3BAVIE  0x10
 #define bmIN2BAVIE  0x08
 #define bmOUT1DAVIE 0x04
 #define bmOUT0DAVIE 0x02
 #define bmIN0BAVIE  0x01
 
// R13 USBIRQ register bits
 #define bmURESDNIRQ 0x80
 #define bmVBUSIRQ   0x40
 #define bmNOVBUSIRQ 0x20
 #define bmSUSPIRQ   0x10
 #define bmURESIRQ   0x08
 #define bmBUSACTIRQ 0x04
 #define bmRWUDNIRQ  0x02
 #define bmOSCOKIRQ  0x01
 
// R14 USBIEN register bits
 #define bmURESDNIE  0x80
 #define bmVBUSIE    0x40
 #define bmNOVBUSIE  0x20
 #define bmSUSPIE    0x10
 #define bmURESIE    0x08
 #define bmBUSACTIE  0x04
 #define bmRWUDNIE   0x02
 #define bmOSCOKIE   0x01
 
// R15 USBCTL Register
 #define bmHOSCSTEN  0x80
 #define bmVBGATE    0x40
 #define bmCHIPRES   0x20
 #define bmPWRDOWN   0x10
 #define bmCONNECT   0x08
 #define bmSIGRWU    0x04
 
// R16 CPUCTL Register
 #define bmIE        0x01
 
// R17 PINCTL Register
 #define bmFDUPSPI   0x10
 #define bmINTLEVEL  0x08
 #define bmPOSINT    0x04
 #define bmGPOB      0x02
 #define  bmGPOA      0x01
 
//
// GPX[B:A] settings (PINCTL register)
 #define gpxOPERATE  0x00
 #define gpxVBDETECT 0x01
 #define gpxBUSACT   0x02
 #define gpxSOF      0x03
 
// ************************
// Standard USB Requests
 #define SR_GET_STATUS 0x00 // Get Status
 #define SR_CLEAR_FEATURE 0x01 // Clear Feature
 #define SR_RESERVED 0x02 // Reserved
 #define SR_SET_FEATURE 0x03 // Set Feature
 #define SR_SET_ADDRESS 0x05 // Set Address
 #define SR_GET_DESCRIPTOR 0x06 // Get Descriptor
 #define SR_SET_DESCRIPTOR 0x07 // Set Descriptor
 #define SR_GET_CONFIGURATION 0x08 // Get Configuration
 #define SR_SET_CONFIGURATION 0x09 // Set Configuration
 #define SR_GET_INTERFACE 0x0a // Get Interface
 #define SR_SET_INTERFACE 0x0b // Set Interface
 
// Get Descriptor codes
 #define GD_DEVICE 0x01 // Get device descriptor: Device
 #define GD_CONFIGURATION 0x02 // Get device descriptor: Configuration
 #define GD_STRING 0x03 // Get device descriptor: String
 #define GD_REPORT 0x22 // Get descriptor: Report
 #define CS_INTERFACE 0x24 // Get descriptor: Interface
 #define CS_ENDPOINT 0x25 // Get descriptor: Endpoint
 
// SETUP packet offsets
 #define bmRequestType 0
 #define bRequest 1
 #define wValueL 2
 #define wValueH 3
 #define wIndexL 4
 #define wIndexH 5
 #define wLengthL 6
 #define wLengthH 7
 
// CDC bRequest values
 #define SEND_ENCAPSULATED_COMMAND 0x00
 #define GET_ENCAPSULATED_RESPONSE 0x01
 #define SET_COMM_FEATURE 0x02
 #define GET_COMM_FEATURE 0x03
 #define CLEAR_COMM_FEATURE 0x04
 #define SET_LINE_CODING 0x20
 #define GET_LINE_CODING 0x21
 #define SET_CONTROL_LINE_STATE 0x22
 #define SEND_BREAK 0x23

static uint8_t maxWriteByte(uint8_t command, uint8_t data)
{
    uint32_t readresA = 0;
    uint32_t readresB = 0;

    command |= 0x02;                         // set write bit

    UARTWrite("W");
    *IO_USBCCSn = 0x0;
    //MAX_ASSERT_CS;

    //MAX_USART.STATUS = USART_TXCIF_bm;     // clear TX-complete flag
    UARTWrite("C");
    *IO_USBCTRX = command;                   // send command byte
    UARTWrite("D");
    *IO_USBCTRX = data;                      // send data

    //waitForTxComplete();

    //MAX_RELEASE_CS;
    *IO_USBCCSn = 0x1;

    UARTWrite("e");

    UARTWrite("H:");
    uint32_t cntr = 0;
    uint32_t R = 0xFF;
    while(R==0xFF && (cntr++<255)) { R=*IO_USBCTRX; }

    readresA = R;           // status byte
    readresB = *IO_USBCTRX; // discard dummy byte

    UARTWriteHexByte(readresA);
    UARTWrite("\n");

    return readresA;
}

static uint8_t maxReadByte(uint8_t command)
{
    volatile uint32_t readresA = 0;
    volatile uint32_t readresB = 0;

    //MAX_ASSERT_CS;
    UARTWrite("R:");
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

    UARTWriteHexByte(readresB);
    UARTWrite("\n");

    return readresB;
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

int main()
{
    UARTWrite("USB-C test\n");
    //initMax3420();
    //initUsb();

    maxWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF);
    maxWriteByte(rGPIO, 0x0);
    maxWriteByte(rUSBCTL, bmCONNECT | bmVBGATE);
    maxWriteByte(rUSBIEN, bmURESIE | bmURESDNIE);
    maxWriteByte(rCPUCTL, bmIE);

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
