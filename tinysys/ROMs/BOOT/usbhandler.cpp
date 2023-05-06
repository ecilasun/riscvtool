#include "usbhandler.h"
#include "usbdata.h"
#include "leds.h"
#include "uart.h"

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

static uint8_t configval = 0;

void set_configuration(uint8_t *SUD)
{
    configval = SUD[wValueL];       // Store the config value
    if(configval != 0)              // If we are configured, 
        SETBIT(rUSBIEN, bmSUSPIE);  // start looking for SUSPEND interrupts
    USBReadByte(rFNADDR | 0x1);     // dummy read to set the ACKSTAT bit
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
	switch (SUD[wValueH])			// wValueH is descriptor type
	{
	    case  GD_DEVICE:
              desclen = DD[0];	// descriptor length
              pDdata = DD;
              break;	
	    case  GD_CONFIGURATION:
              desclen = CD[2];	// Config descriptor includes interface, HID, report and ep descriptors
              pDdata = CD;
              break;
	    case  GD_STRING:
              desclen = strDesc[SUD[wValueL]][0];   // wValueL=string index, array[0] is the length
              pDdata = strDesc[SUD[wValueL]];       // point to first array element
              break;
	    case  GD_HID:
              desclen = CD[18];
              pDdata = &CD[18];
              break;
	    case  GD_REPORT:
              desclen = CD[25];
              pDdata = RepD;
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
	    case	SR_SET_FEATURE:		    UARTWrite("feature(1)");           break;
	    case	SR_CLEAR_FEATURE:	    UARTWrite("feature(0)");           break;
	    case	SR_GET_STATUS:		    UARTWrite("get_status()");         break;
	    case	SR_SET_INTERFACE:	    UARTWrite("set_interface()");      break;
	    case	SR_GET_INTERFACE:	    UARTWrite("get_interface()");      break;
	    case	SR_GET_CONFIGURATION:   UARTWrite("get_configuration()");  break;
	    case	SR_SET_CONFIGURATION:   set_configuration(SUD);            break;
	    case	SR_SET_ADDRESS:         USBReadByte(rFNADDR | 0x1);        break;  // discard return value
	    default:  STALL_EP0
	}
}

void class_request(uint8_t *SUD)
{
    STALL_EP0
}                         

void vendor_request(uint8_t *SUD)
{
    STALL_EP0
}

void HandleUSBC()
{
	uint32_t currLED = LEDGetState();
	// Clear top 2 bits
	currLED &= 0x3;

	uint8_t epIrq = USBReadByte(rEPIRQ);
	uint8_t usbIrq = USBReadByte(rUSBIRQ);

	if (usbIrq & bmURESDNIRQ)
	{
		currLED |= 0x8;
		USBWriteByte(rUSBIRQ, bmURESDNIRQ);
		USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE);
	}

	if (epIrq & bmSUDAVIRQ)
	{
		USBWriteByte(rEPIRQ, bmSUDAVIRQ); // clear SUDAV irq

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
			case 0x20: class_request(SUD); break;
			case 0x40: vendor_request(SUD); break;
			default: STALL_EP0 break;
		}
		currLED |= 0x4;
	}

	if (usbIrq & bmURESIRQ)
	{
		USBWriteByte(rUSBIRQ, bmURESIRQ);
	}

	if (epIrq & bmIN3BAVIRQ)
	{
		// TODO:
	}

	if (usbIrq & bmSUSPIRQ)
	{
		USBWriteByte(rUSBIRQ, bmSUDAVIRQ | bmBUSACTIRQ);
	}

	LEDSetState(currLED);
}
