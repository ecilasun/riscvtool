#include "basesystem.h"
#include "uart.h"
#include "usbc.h"
#include "leds.h"

// See:
// https://github.com/MicrochipTech/mla_usb/blob/master/src/usb_device_cdc.c

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

// 19200 baud, 1 stop bit, no parity, 8bit data
static USBCDCLineCoding s_lineCoding{19200,0,0,8};

uint8_t configval = 0;

void set_configuration(uint8_t *SUD)
{
    configval = SUD[wValueL];       // Store the config value

    UARTWrite(" set_configuration: 0x");
    UARTWriteHexByte(configval);
    UARTWrite("\n");

    if(configval != 0)              // If we are configured, 
        SETBIT(rUSBIEN, bmSUSPIE);  // start looking for SUSPEND interrupts
    USBReadByte(rFNADDR | 0x1);     // dummy read to set the ACKSTAT bit
}

void get_configuration(void)
{
	USBWriteByte(rEP0FIFO, configval);	// Send the config value
	USBWriteByte(rEP0BC | 0x1, 1);
}

void send_descriptor(uint8_t *SUD)
{
    uint16_t reqlen, sendlen, desclen;
    unsigned char *pDdata;			// pointer to ROM Descriptor data to send
    //
    // NOTE This function assumes all descriptors are 64 or fewer bytes and can be sent in a single packet
    //
    desclen = 0;					// check for zero as error condition (no case statements satisfied)
    reqlen = SUD[wLengthL] + 256*SUD[wLengthH];	// 16-bit

    // Access descriptor from usb utils
    struct SUSBContext *uctx = USBGetContext();

    uint8_t desctype = SUD[wValueH];
    UARTWrite(" send_descriptor: 0x");
    UARTWriteHexByte(desctype);
    UARTWrite("\n");

	switch (desctype)
	{
	    case  GD_DEVICE:
        {
              desclen = uctx->device.bLength;
              pDdata = (unsigned char*)&uctx->device;
              break;
        }
	    case  GD_CONFIGURATION:
        {
              desclen = uctx->config.wTotalLength;
              pDdata = (unsigned char*)&uctx->config;
              break;
        }
	    case  GD_STRING:
        {
              uint8_t idx = SUD[wValueL];  // String index
              desclen = uctx->strings[idx].bLength;
              pDdata = (unsigned char*)&uctx->strings[idx];
              break;
        }
	}	// end switch on descriptor type

    if (desclen != 0) // one of the case statements above filled in a value
	{
	    sendlen = (reqlen <= desclen) ? reqlen : desclen; // send the smaller of requested and avaiable
        USBWriteBytes(rEP0FIFO,sendlen,pDdata);
	    USBWriteByte(rEP0BC | 0x1, sendlen);   // load EP0BC to arm the EP0-IN transfer & ACKSTAT
	}
    else
        STALL_EP0  // none of the descriptor types match
}

void std_request(uint8_t *SUD)
{
    UARTWrite("std_request: 0x");
    UARTWriteHexByte(SUD[bRequest]);
    UARTWrite("\n");

    switch(SUD[bRequest])
	{
	    case	SR_GET_DESCRIPTOR:	    send_descriptor(SUD);                 break;
	    case	SR_SET_FEATURE:		    UARTWrite(" feature(1)\n");           break;
	    case	SR_CLEAR_FEATURE:	    UARTWrite(" feature(0)\n");           break;
	    case	SR_GET_STATUS:		    UARTWrite(" get_status()\n");         break;
	    case	SR_SET_INTERFACE:	    UARTWrite(" set_interface()\n");      break;
	    case	SR_GET_INTERFACE:	    UARTWrite(" get_interface()\n");      break;
	    case	SR_GET_CONFIGURATION:   get_configuration();                  break;
	    case	SR_SET_CONFIGURATION:   set_configuration(SUD);               break;
	    case	SR_SET_ADDRESS:
        {
            UARTWrite(" setaddress: 0x");
            uint32_t addr = USBReadByte(rFNADDR | 0x1);
            UARTWriteHexByte(addr);
            UARTWrite("\n");
            break;
        }
	    default: STALL_EP0 break;
	}
}

void class_request(uint8_t *SUD)
{
    UARTWrite("class_request: 0x");
    UARTWriteHexByte(SUD[bRequest]);
    UARTWrite("\n");

    // Microchip AN1247
    // https://www.microchip.com/content/dam/mchp/documents/OTH/ProductDocuments/LegacyCollaterals/01247a.pdf
    //
    // req  type value length       data                            name
    // 0    0x21 0     numdatabytes control protocol based command  sendencapsulatedcommand
    // 1    0xA1 0     numdatabytes protocol dependent data         getencapsulatedresponse
    // 0x20 0x21 0     7            line coding data                setlinecoding
    // 0x21 0xA1 0     7            line coding data                getlinecoding
    // 0x22 0x21 2     0            none                            setcontrollinestate

    uint16_t reqlen = SUD[wLengthL] + 256*SUD[wLengthH];	// 16-bit

    switch(SUD[bRequest])
    {
        case 0x00:
        {
            // Command issued
            UARTWrite(" sendencapsulatedcommand 0x");
            UARTWriteHexByte(SUD[bmRequestType]);
            UARTWrite(" 0x");
            UARTWriteHex(reqlen);
            UARTWrite("\n");

            // RESPONSE_AVAILABLE(0x00000001) + 0x00000000
            // or
            // just two zeros for no response
            uint8_t dummyresponse[0x08] = {};
            USBWriteBytes(rEP0FIFO, 0x08, dummyresponse);
            USBWriteByte(rEP0BC | 0x1, 0x08);

            break;
        }
        case 0x01:
        {
            // Response requested
            UARTWrite(" getencapsulatedresponse 0x");
            UARTWriteHexByte(SUD[bmRequestType]);
            UARTWrite(" 0x");
            UARTWriteHex(reqlen);
            UARTWrite("\n");

            // When unhandled, respond with a one-byte zero and do not stall the endpoint
            USBWriteByte(rEP0FIFO, 0);
            USBWriteByte(rEP0BC | 0x1, 1);

            break;
        }
        case 0x20:
        {
            // Data rate/parity/number of stop bits etc
            UARTWrite(" setlinecoding 0x");
            UARTWriteHexByte(SUD[bmRequestType]);
            UARTWrite("\n");
            STALL_EP0
            break;
        }
        case 0x21:
        {
            // Data rate/parity/number of stop bits etc
            // offset name        size description
            // 0      dwDTERate   4    rate in bits per second
            // 4      bCharFormat 1    stop bits: 0:1, 1:1.5, 2:2
            // 5      bParityType 1    parity: 0:none,1:odd,2:even,3:mark,4:space
            // 6      bDataBits   1    data bits: 5,6,7,8 or 16
            UARTWrite(" getlinecoding 0x");
            UARTWriteHexByte(SUD[bmRequestType]);
            UARTWrite("\n");

            USBWriteBytes(rEP0FIFO, sizeof(USBCDCLineCoding), (uint8_t*)&s_lineCoding);
            USBWriteByte(rEP0BC | 0x1, sizeof(USBCDCLineCoding));

            break;
        }
        case 0x22:
        {
            // bits  description
            // 15:2  reserved
            // 1     carrier control signal: 0:inactive,1:active
            // 0     DTR: 0:notpresent, 1:present
            UARTWrite(" setcontrollinestate 0x");
            UARTWriteHexByte(SUD[bmRequestType]);
            UARTWrite("\n");

            STALL_EP0

            break;
        }
    }
}

void DoSetup()
{
    uint8_t SUD[8];
    USBReadBytes(rSUDFIFO, 8, SUD);

    if (SUD[0] != 0xFF)
    {
        UARTWriteHexByte(SUD[0]);
        UARTWriteHexByte(SUD[1]);
        UARTWriteHexByte(SUD[2]);
        UARTWriteHexByte(SUD[3]);
        UARTWriteHexByte(SUD[4]);
        UARTWriteHexByte(SUD[5]);
        UARTWriteHexByte(SUD[6]);
        UARTWriteHexByte(SUD[7]);
        UARTWrite("\n");
    }

    switch(SUD[bmRequestType] & 0x60)
    {
        case 0x00: std_request(SUD); break;
        case 0x20: class_request(SUD); break; // class_req
        case 0x40: UARTWrite("vendor_req()\n"); STALL_EP0 break; // vendor_req
        default:
        {
            UARTWrite("unknown_req(): 0x");
            UARTWriteHexByte(SUD[bmRequestType] & 0x60);
            UARTWrite("\n");
            STALL_EP0
            break;
        }
    }

}

int main(int argc, char *argv[])
{
    UARTWrite("USB-C test\n");

    if (argc>1)
    {
        UARTWrite("Bringing up USB-C\nUsing ISR in ROM\n");
        USBInit(1);
        UARTWrite("USB ISR will handle further communications.\n");
    }
    else
    {
        UARTWrite("Bringing up USB-C\nUsing polling\n");
        USBInit(0);

        // Ordinarily ROM listens to this
        while (1)
        {
            /*if (Suspended)
                check_for_resume();
            
            if (MAX_Int_Pending()
            {*/

            uint32_t currLED = LEDGetState();
            LEDSetState(currLED | 0x8);

            // Initial value of rEPIRQ should be 0x19
	        uint8_t epIrq = USBReadByte(rEPIRQ);
            uint8_t usbIrq = USBReadByte(rUSBIRQ);

            /*if (epIrq != 0xFF && usbIrq != 0xFF)
            {
                UARTWriteHexByte(epIrq);
                UARTWrite(":");
                UARTWriteHexByte(usbIrq);
                UARTWrite(":");
                UARTWriteHexByte(USBGetGPX());
                UARTWrite("\n");
            }*/

            if (epIrq & bmSUDAVIRQ)
            {
    		    USBWriteByte(rEPIRQ, bmSUDAVIRQ); // clear SUDAV irq
        		// Setup data available, 8 bytes data to follow
		        DoSetup();
            }

            if (epIrq & bmIN3BAVIRQ)
            {
                // TODO: asserts out of reset
                // According to app notes we can't directly clear BAV bits
                USBWriteByte(rEPIRQ, bmIN3BAVIRQ); // Clear
                // do_IN3();
            }

            if ((configval != 0) && (usbIrq & bmSUSPIRQ)) // Suspend
            {
                // Should arrive here out of reset
                USBWriteByte(rUSBIRQ, bmSUSPIRQ | bmBUSACTIRQ); // Clear
                // Suspended = 1
            }

            if (usbIrq & bmURESIRQ) // Bus reset
            {
                USBWriteByte(rUSBIRQ, bmURESIRQ); // Clear
            }

            if (usbIrq & bmURESDNIRQ) // Resume
            {
                USBWriteByte(rUSBIRQ, bmURESDNIRQ); // clear URESDN irq
                USBWriteByte(rEPIEN, bmSUDAVIE | bmIN3BAVIE);
                // Suspended=0;
                USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE | bmSUSPIE);
            }

            /*if (epIrq & bmIN2BAVIRQ)
            {
                // TODO: asserts out of reset
                USBWriteByte(rEPIRQ, bmIN2BAVIRQ); // Clear
            }

            if (epIrq & bmIN0BAVIRQ)
            {
                // TODO: asserts out of reset
                USBWriteByte(rEPIRQ, bmIN0BAVIRQ); // Clear
            }*/

            LEDSetState(currLED);
        }
    }

    return 0;
}


// From Bringing up USB using MAX3420
/*UARTWrite("Testing IEN\nShould see: 01 02 04 08 10 20 40 80\n");
uint8_t wr = 0x01; // initial register write value 
for(int j=0; j<8; j++) 
{ 
    USBWriteByte(rUSBIEN, wr);

    uint8_t rd = USBReadByte(rUSBIEN);

    UARTWriteHexByte(rd);
    UARTWrite(" ");

    wr <<= 1; // Put a breakpoint here. Values of 'rd' should be 01,02,04,08,10,20,40,80 
}
UARTWrite("\n");*/
