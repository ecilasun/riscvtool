#include "usbchandler.h"
#include "leds.h"
#include "uart.h"

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

static uint8_t s_suspended = 0;
static uint8_t configval = 0; // None?

void set_configuration(uint8_t *SUD)
{
	// Set configuration: 1-> control

    configval = SUD[wValueL];       // Store the config value
    if(configval != 0)              // If we are configured, 
        SETBIT(rUSBIEN, bmSUSPIE);  // start looking for SUSPEND interrupts
    USBReadByte(rFNADDR | 0x1);     // dummy read to set the ACKSTAT bit
}

void get_configuration(void)
{
	USBWriteByte(rEP0FIFO, configval);	// Send the current config value
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
	switch (desctype)
	{
	    case  GD_DEVICE:
              desclen = uctx->device.bLength;
              pDdata = (unsigned char*)&uctx->device;
              break;	
	    case  GD_CONFIGURATION:
              desclen = uctx->config.wTotalLength;
              pDdata = (unsigned char*)&uctx->config;
              break;
	    case  GD_STRING:
              uint8_t idx = SUD[wValueL];  // String index
              desclen = uctx->strings[idx].bLength;
              pDdata = (unsigned char*)&uctx->strings[idx];
              break;
	}	// end switch on descriptor type

    if (desclen!=0) // one of the case statements above filled in a value
	{
	    sendlen = (reqlen <= desclen) ? reqlen : desclen; // send the smaller of requested and avaiable
        //writebytes(rEP0FIFO,sendlen,pDdata);
        USBWriteBytes(rEP0FIFO,sendlen,pDdata);
	    USBWriteByte(rEP0BC | 0x1, sendlen);   // load EP0BC to arm the EP0-IN transfer & ACKSTAT
	}
    else
        STALL_EP0  // none of the descriptor types match
}

void std_request(uint8_t *SUD)
{
    switch(SUD[bRequest])
	{
	    case	SR_GET_DESCRIPTOR:	    send_descriptor(SUD);              break;
	    case	SR_SET_FEATURE:		    UARTWrite("feature(1)\n");         break;
	    case	SR_CLEAR_FEATURE:	    UARTWrite("feature(0)\n");         break;
	    case	SR_GET_STATUS:		    UARTWrite("get_status()\n");       break;
	    case	SR_SET_INTERFACE:	    UARTWrite("set_interface()\n");    break;
	    case	SR_GET_INTERFACE:	    UARTWrite("get_interface()\n");    break;
	    case	SR_GET_CONFIGURATION:   get_configuration();               break;
	    case	SR_SET_CONFIGURATION:   set_configuration(SUD);            break;
	    case	SR_SET_ADDRESS:
        {
            /*UARTWrite("setaddress: 0x");
            uint32_t addr =*/ USBReadByte(rFNADDR | 0x1);
            /*UARTWriteHexByte(addr);
            UARTWrite("\n");*/
			// TODO: Do we need to actually do anything here?
            break;
        }
	    default:  STALL_EP0
	}
}

void class_request(uint8_t *SUD)
{
	// CDC class specific request
    /*UARTWrite("class_request: 0x");
    UARTWriteHexByte(SUD[bRequest]);
    UARTWrite("\n");*/

    /*switch(SUD[bRequest])
	{
	    case	SR_GET_DESCRIPTOR:	    send_descriptor(SUD);              break;
	    case	SR_SET_FEATURE:		    UARTWrite("feature(1)");           break;
	    case	SR_CLEAR_FEATURE:	    UARTWrite("feature(0)");           break;
	    case	SR_GET_STATUS:		    UARTWrite("get_status()");         break;
	    case	SR_SET_INTERFACE:	    UARTWrite("set_interface()");      break;
	    case	SR_GET_INTERFACE:	    UARTWrite("get_interface()");      break;
	    case	SR_GET_CONFIGURATION:   get_configuration();               break;
	    case	SR_SET_CONFIGURATION:   set_configuration(SUD);            break;
	    case	SR_SET_ADDRESS:
        {
            UARTWrite("setaddress: 0x");
            uint32_t addr = USBReadByte(rFNADDR | 0x1);
            UARTWriteHexByte(addr);
            UARTWrite("\n");
            break;
        }
	    default: STALL_EP0 break;
	}*/

    STALL_EP0
}                         

void vendor_request(uint8_t *SUD)
{
    STALL_EP0
}

void DoSetup()
{
	uint8_t SUD[8];
	USBReadBytes(rSUDFIFO, 8, SUD);

	switch(SUD[bmRequestType] & 0x60)
	{
		case 0x00: std_request(SUD); break;
		case 0x20: class_request(SUD); break;
		case 0x40: vendor_request(SUD); break;
		default: STALL_EP0 break;
	}
}

void HandleUSBC()
{
	uint32_t currLED = LEDGetState();
	LEDSetState(currLED | 0x8);

	// Initial value of rEPIRQ should be 0x19
	uint8_t epIrq = USBReadByte(rEPIRQ);
	uint8_t usbIrq = USBReadByte(rUSBIRQ);

	// Debug - We don't do this in ROM mode since it'll be called from an ISR
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
		// According to app notes we can't directly clear BAV bits
		USBWriteByte(rEPIRQ, bmIN3BAVIRQ); // Clear
		// do_IN3();
	}

	if ((configval != 0) && (usbIrq & bmSUSPIRQ)) // Suspend
	{
		// Should arrive here out of reset
		USBWriteByte(rUSBIRQ, bmSUSPIRQ | bmBUSACTIRQ); // Clear
		s_suspended = 1;
	}

	if (usbIrq & bmURESIRQ) // Bus reset
	{
		USBWriteByte(rUSBIRQ, bmURESIRQ); // Clear
	}

	if (usbIrq & bmURESDNIRQ) // Resume
	{
		USBWriteByte(rUSBIRQ, bmURESDNIRQ);
		USBWriteByte(rEPIEN, bmSUDAVIE | bmIN3BAVIE);
		s_suspended = 0;
		// Re-enable interrupts since bus reset clears them
		USBWriteByte(rUSBIEN, bmURESDNIE | bmURESIE | bmSUSPIE);
	}

	LEDSetState(currLED);
}
