#include "usbhandler.h"
#include "usbdata.h"
#include "leds.h"
#include "uart.h"

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

static uint8_t s_suspended = 0;
static uint8_t configval = 0;
//static uint8_t inhibit_send = 0;

void set_configuration(uint8_t *SUD)
{
    configval = SUD[wValueL];       // Store the config value
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
	    case	SR_GET_CONFIGURATION:   get_configuration();               break;
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

void DoSetup()
{
	uint8_t SUD[8];
	USBReadBytes(rSUDFIFO, 8, SUD);

	// Debug dump
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
}

void DoIN3()
{
	/*if (inhibit_send == 0x01) {
		USBWriteByte(rEP3INFIFO, 0); // send the "keys up" code
		USBWriteByte(rEP3INFIFO, 0);
		USBWriteByte(rEP3INFIFO, 0);
	} else if (send3zeros == 0x01) { // precede every keycode with the "no keys" code
		USBWriteByte(rEP3INFIFO, 0); // send the "keys up" code
		USBWriteByte(rEP3INFIFO, 0);
		USBWriteByte(rEP3INFIFO, 0);
		send3zeros = 0; // next time through this function send the keycode
	} else {
		send3zeros=1;
		USBWriteByte(rEP3INFIFO,Message[msgidx++]);	// load the next keystroke (3 bytes)
		USBWriteByte(rEP3INFIFO,Message[msgidx++]);
		USBWriteByte(rEP3INFIFO,Message[msgidx++]);
	if(msgidx >= msglen)                    // check for message wrap
			{
			msgidx=0;
			inhibit_send=1;                 // send the string once per pushbutton press
			}
	}
	USBWriteByte(rEP3INBC, 3); // arm it*/
}

void HandleUSBC()
{
	uint32_t currLED = LEDGetState();
	LEDSetState(currLED | 0x8);

	// Initial value of rEPIRQ should be 0x19
	uint8_t epIrq = USBReadByte(rEPIRQ);
	uint8_t usbIrq = USBReadByte(rUSBIRQ);

	// Debug
	if (epIrq != 0xFF && usbIrq != 0xFF)
	{
		UARTWriteHexByte(epIrq);
		UARTWrite(":");
		UARTWriteHexByte(usbIrq);
		UARTWrite(":");
		UARTWriteHexByte(USBGetGPX());
		UARTWrite("\n");
	}

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

		DoIN3();
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

		s_suspended = 0;
		// Re-enable interrupts since bus reset clears them
		USBWriteByte(rUSBIEN, bmURESDNIE | bmURESIE | bmSUSPIE);
		USBWriteByte(rEPIEN, bmSUDAVIE | bmIN3BAVIE);
	}

	LEDSetState(currLED);
}
