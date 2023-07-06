#include "basesystem.h"
#include "uart.h"
#include "usbserial.h"
#include "leds.h"
#include <malloc.h>

// See:
// https://github.com/MicrochipTech/mla_usb/blob/master/src/usb_device_cdc.c
// https://github.com/tlh24/myopen/blob/master/firmware_stage7/usb.c

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

// 19200 baud, 1 stop bit, no parity, 8bit data
static USBCDCLineCoding s_lineCoding{19200, 0, 0, 8};

static uint8_t s_outputbuffer[64];
static uint32_t s_outputbufferlen = 0;

static uint8_t s_inputbuffer[64];
static uint32_t s_inputbufferlen = 0;

static uint32_t s_suspended = 0;

static uint8_t devconfig = 0;
static uint8_t devaddrs = 0;
static uint8_t encapsulatedcommand[0x20];
static uint32_t addrx0x81stalled = 0;

void set_configuration(uint8_t *SUD)
{
	devconfig = SUD[wValueL];	   // Store the config value

	UARTWrite("set_configuration: 0x");
	UARTWriteHexByte(devconfig);
	UARTWrite("\n");

	if(devconfig != 0)			  // If we are configured, 
		SETBIT(rUSBIEN, bmSUSPIE);  // start looking for SUSPEND interrupts
	USBReadByte(rFNADDR | 0x1);	 // dummy read to set the ACKSTAT bit
}

void get_configuration(void)
{
	UARTWrite("get_configuration: 0x");
	UARTWriteHexByte(devconfig);
	UARTWrite("\n");

	USBWriteByte(rEP0FIFO, devconfig);	// Send the config value
	USBWriteByte(rEP0BC | 0x1, 1);
}

void send_status(uint8_t *SUD)
{
	UARTWrite("send_status\n");

	// Device: 0x0 -> bus powered, no remove wakeup
	// Interface: must be zero
	// Endpoint: must be zero
	uint8_t twozero[2] = {0, 0};
	USBWriteBytes(rEP0FIFO, 2, (uint8_t*)&twozero);
	USBWriteByte(rEP0BC | 0x1, 2);
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
	UARTWrite("send_descriptor: ");

	switch (desctype)
	{
		case  GD_DEVICE:
		{
			UARTWrite("device");
			desclen = uctx->device.bLength;
			pDdata = (unsigned char*)&uctx->device;
			break;
		}
		case  GD_CONFIGURATION:
		{
			UARTWrite("config");
			desclen = uctx->config.wTotalLength;
			pDdata = (unsigned char*)&uctx->config;
			break;
		}
		case  GD_STRING:
		{
			UARTWrite("string 0x");
			uint8_t idx = SUD[wValueL];  // String index
			UARTWriteHexByte(idx);
			desclen = uctx->strings[idx].bLength;
			pDdata = (unsigned char*)&uctx->strings[idx];
			break;
		}
	}	// end switch on descriptor type

	if (desclen != 0) // one of the case statements above filled in a value
	{
		sendlen = (reqlen <= desclen) ? reqlen : desclen; // send the smaller of requested and avaiable
		USBWriteBytes(rEP0FIFO, sendlen, pDdata);
		USBWriteByte(rEP0BC | 0x1, sendlen);   // load EP0BC to arm the EP0-IN transfer & ACKSTAT
	}
	else
		STALL_EP0  // none of the descriptor types match

	UARTWrite("\n");
}

void featurecontrol(int ctltype, uint16_t value, uint16_t index)
{
	// ctltype == 0 -> clear
	// ctltype == 1 -> set

	if (value == 0x00) // ENDPOINT_HALT
	{
		UARTWrite("endpoint_halt: 0x");
		UARTWriteHexByte(index);

		uint8_t stallmask = USBReadByte(rEPSTALLS);
		if (ctltype == 1) // Set
		{
			if (index==0x81) // Control
			{
				stallmask |= bmSTLEP1OUT;
				UARTWrite(" -> halted");
				addrx0x81stalled = 1;
			}
			else
				UARTWrite(" -> can't halt");
		}
		else // Clear
		{
			if (index==0x81) // Control
			{
				stallmask &= ~bmSTLEP1OUT;
				UARTWrite(" -> resumed");
				addrx0x81stalled = 0;
				USBWriteByte(rCLRTOGS, bmCTGEP1OUT);
			}
			else
				UARTWrite(" -> can't resume");
		}
		UARTWrite("\n");
		USBWriteByte(rEPSTALLS, stallmask | bmACKSTAT);
	}
	else if (value == 0x01) // REMOTE_WAKEUP
	{
		uint8_t addrs = USBReadByte(rFNADDR);
		UARTWrite("remote_wakeup 0x");
		UARTWriteHexByte(addrs);
		UARTWrite("\n");
	}
	else // Unknown
	{
		UARTWrite("unknown feature control\n");
		STALL_EP0
	}
}

void std_request(uint8_t *SUD)
{
	switch(SUD[bRequest])
	{
		case	SR_GET_DESCRIPTOR:
		{
			send_descriptor(SUD);
			break;
		}
		case	SR_SET_FEATURE:
		{
			uint16_t value = (SUD[wValueH]<<8) | SUD[wValueL];
			uint16_t index = (SUD[wIndexH]<<8) | SUD[wIndexL];

			UARTWrite("set_feature type:0x");
			UARTWriteHexByte(SUD[bmRequestType]);
			UARTWrite("\n");

			switch (SUD[bmRequestType])
			{
				// EP0
				case 0x00: STALL_EP0 break;
				// Interface
				case 0x01: USBWriteByte(rEP0BC | 0x1, 0); break; // Zero byte response - ACK
				// Endpoint
				case 0x02: featurecontrol(1, value, index); break;
				// Unknown
				default: STALL_EP0 break;
			}
			break;
		}
		case	SR_CLEAR_FEATURE:
		{
			uint16_t value = (SUD[wValueH]<<8) | SUD[wValueL];
			uint16_t index = (SUD[wIndexH]<<8) | SUD[wIndexL];

			UARTWrite("clear_feature type:0x");
			UARTWriteHexByte(SUD[bmRequestType]);
			UARTWrite("\n");

			switch (SUD[bmRequestType])
			{
				// EP0
				case 0x00: STALL_EP0 break;
				// Interface
				case 0x01: USBWriteByte(rEP0BC | 0x1, 0); break; // Zero byte response - ACK
				// Endpoint
				case 0x02: featurecontrol(0, value, index); break;
				// Unknown
				default: STALL_EP0 break;
			}
			break;
		}
		case	SR_GET_STATUS:			send_status(SUD);					break;
		case	SR_SET_INTERFACE:		UARTWrite("!set_interface\n");	   break;
		case	SR_GET_INTERFACE:		UARTWrite("!get_interface\n");	   break;
		case	SR_GET_CONFIGURATION:   get_configuration();				 break;
		case	SR_SET_CONFIGURATION:   set_configuration(SUD);			  break;
		case	SR_SET_ADDRESS:
		{
			UARTWrite("setaddress: 0x");
			devaddrs = USBReadByte(rFNADDR | 0x1);
			UARTWriteHexByte(devaddrs);
			UARTWrite("\n");
			break;
		}
		default:
		{
			UARTWrite("unimplemented std_request: 0x");
			UARTWriteHexByte(SUD[bRequest]);
			UARTWrite("\n");
			STALL_EP0
			break;
		}
	}
}

void class_request(uint8_t *SUD)
{
	// Microchip AN1247
	// https://www.microchip.com/content/dam/mchp/documents/OTH/ProductDocuments/LegacyCollaterals/01247a.pdf
	//
	// req  type value length	   data							name
	// 0	0x21 0	 numdatabytes control protocol based command  sendencapsulatedcommand
	// 1	0xA1 0	 numdatabytes protocol dependent data		 getencapsulatedresponse
	// 0x20 0x21 0	 7			line coding data				setlinecoding
	// 0x21 0xA1 0	 7			line coding data				getlinecoding
	// 0x22 0x21 2	 0			none							setcontrollinestate

	uint16_t reqlen = SUD[wLengthL] + 256*SUD[wLengthH];	// 16-bit

	switch(SUD[bRequest])
	{
		case CDC_SENDENCAPSULATEDRESPONSE:
		{
			// Command issued
			UARTWrite("sendencapsulatedcommand 0x");
			UARTWriteHexByte(SUD[bmRequestType]);
			UARTWrite(" 0x");
			UARTWriteHex(reqlen);
			UARTWrite(":");

			// RESPONSE_AVAILABLE(0x00000001) + 0x00000000
			// or
			// just two zeros for no response
			USBReadBytes(rEP0FIFO, reqlen, encapsulatedcommand);
			for (uint16_t i=0;i<reqlen;++i)
				UARTWriteHex(encapsulatedcommand[i]);
			UARTWrite("\n");

			USBWriteByte(rEP0BC | 0x1, 0); // Zero byte response - ACK
			/*uint8_t noresponse[2] = {0,0};
			USBWriteBytes(rEP0FIFO, 2, noresponse);
			USBWriteByte(rEP0BC | 0x1, 2);*/

			break;
		}
		case CDC_GETENCAPSULATEDRESPONSE:
		{
			// Response requested
			UARTWrite("getencapsulatedresponse 0x");
			UARTWriteHexByte(SUD[bmRequestType]);
			UARTWrite(" 0x");
			UARTWriteHex(reqlen);
			UARTWrite("\n");

			// When unhandled, respond with a one-byte zero and do not stall the endpoint
			USBWriteByte(rEP0FIFO, 0);
			USBWriteByte(rEP0BC | 0x1, 1);

			break;
		}
		case CDC_SETLINECODING:
		{
			// Data rate/parity/number of stop bits etc
			UARTWrite("setlinecoding 0x");
			UARTWriteHexByte(SUD[bmRequestType]);

			USBCDCLineCoding newcoding;
			USBReadBytes(rEP0FIFO, sizeof(USBCDCLineCoding), (uint8_t*)&newcoding);

			UARTWrite(" Rate:");
			UARTWriteDecimal(newcoding.dwDTERate);
			UARTWrite(" Format:");
			UARTWriteHexByte(newcoding.bCharFormat);
			UARTWrite(" Parity:");
			UARTWriteHexByte(newcoding.bParityType);
			UARTWrite(" Bits:");
			UARTWriteHexByte(newcoding.bDataBits);
			UARTWrite("\n");

			s_lineCoding = newcoding;

			USBWriteByte(rEP0BC | 0x1, 0); // Zero byte response - ACK

			break;
		}
		case CDC_GETLINECODING:
		{
			// Data rate/parity/number of stop bits etc
			// offset name		size description
			// 0	  dwDTERate   4	rate in bits per second
			// 4	  bCharFormat 1	stop bits: 0:1, 1:1.5, 2:2
			// 5	  bParityType 1	parity: 0:none,1:odd,2:even,3:mark,4:space
			// 6	  bDataBits   1	data bits: 5,6,7,8 or 16
			UARTWrite("getlinecoding 0x");
			UARTWriteHexByte(SUD[bmRequestType]);

			USBWriteBytes(rEP0FIFO, sizeof(USBCDCLineCoding), (uint8_t*)&s_lineCoding);
			USBWriteByte(rEP0BC | 0x1, sizeof(USBCDCLineCoding));

			UARTWrite(" Rate:");
			UARTWriteDecimal(s_lineCoding.dwDTERate);
			UARTWrite(" Format:");
			UARTWriteHexByte(s_lineCoding.bCharFormat);
			UARTWrite(" Parity:");
			UARTWriteHexByte(s_lineCoding.bParityType);
			UARTWrite(" Bits:");
			UARTWriteHexByte(s_lineCoding.bDataBits);
			UARTWrite("\n");

			break;
		}
		case CDC_SETCONTROLLINESTATE:
		{
			// bits  description
			// 15:2  reserved
			// 1	 carrier control signal: 0:inactive,1:active
			// 0	 DTR: 0:notpresent, 1:present
			UARTWrite("setcontrollinestate 0x");
			UARTWriteHexByte(SUD[bmRequestType]);
			UARTWrite("\n");

			USBWriteByte(rEP0BC | 0x1, 0); // Zero byte response - ACK
			//STALL_EP0

			break;
		}
		default:
		{
			UARTWrite("unknown class_request: 0x");
			UARTWriteHexByte(SUD[bRequest]);
			UARTWrite("\n");
		}
	}
}

void DoSetup()
{
	uint8_t SUD[8];
	USBReadBytes(rSUDFIFO, 8, SUD);

	/*if (SUD[0] != 0xFF)
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
	}*/

	switch(SUD[bmRequestType] & 0x60)
	{
		case 0x00: std_request(SUD); break;
		case 0x20: class_request(SUD); break;
		case 0x40:
		{
			UARTWrite("vendor_req()\n");
			STALL_EP0
			break;
		}
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

void EmitBufferedOutput()
{
	// If we have something pending in the output buffer, stream it out
	if (s_outputbufferlen != 0)
		USBWriteBytes(rEP2INFIFO, s_outputbufferlen, s_outputbuffer);
	USBWriteByte(rEP2INBC, s_outputbufferlen); // Zero or more bytes output
	// Done sending
	s_outputbufferlen = 0;
}

void BufferIncomingData()
{
	// Incoming EP1 data package
	uint8_t cnt = USBReadByte(rEP1OUTBC) & 63; // Cap size to 0..63
	s_inputbufferlen = cnt;
	if (cnt)
		USBReadBytes(rEP1OUTFIFO, cnt, s_inputbuffer);
}

int main(int argc, char *argv[])
{
	UARTWrite("USB-C test\n");

	SUSBContext *newctx = (SUSBContext*)malloc(sizeof(SUSBContext));
	USBSetContext(newctx);

	if (argc>1)
	{
		UARTWrite("Bringing up USB-C\nUsing ISR in ROM\n");
		USBInit(1);

		UARTWrite("MAX3420 die rev# ");
		UARTWriteHexByte(USBReadByte(rRevision));
		UARTWrite("\n");

		UARTWrite("USB ISR will handle further communications.\n");
	}
	else
	{
		UARTWrite("Bringing up USB-C\nUsing polling\n");
		USBInit(0);

		UARTWrite("MAX3420 die rev# ");
		UARTWriteHexByte(USBReadByte(rRevision));
		UARTWrite("\n");

		// Ordinarily ROM listens to this
		while (1)
		{
			uint32_t currLED = LEDGetState();

			// Initial value of rEPIRQ should be 0x19
			uint8_t epIrq = USBReadByte(rEPIRQ);
			uint8_t usbIrq = USBReadByte(rUSBIRQ);

			/*if (epIrq || usbIrq)
			{
				UARTWrite("epIrq 0x");
				UARTWriteHexByte(epIrq);
				UARTWrite(" usbIrq 0x");
				UARTWriteHexByte(usbIrq);
				UARTWrite("\n");
			}*/

			if (epIrq & bmSUDAVIRQ)
			{
				USBWriteByte(rEPIRQ, bmSUDAVIRQ); // Clear
				// Setup data available, 8 bytes data to follow
				LEDSetState(currLED | 0x8);
				DoSetup();
			}

			if (epIrq & bmOUT1DAVIRQ)
			{
				// Input
				BufferIncomingData();
				USBWriteByte(rEPIRQ, bmOUT1DAVIRQ); // Clear
			}

			if (epIrq & bmIN2BAVIRQ)
			{
				// Output
				EmitBufferedOutput();
				USBWriteByte(rEPIRQ, bmIN2BAVIRQ); // Clear
			}

			if (epIrq & bmIN3BAVIRQ)
			{
				USBWriteByte(rEPIRQ, bmIN3BAVIRQ); // Clear
			}

			if (epIrq & bmIN0BAVIRQ)
			{
				USBWriteByte(rEPIRQ, bmIN0BAVIRQ); // Clear
			}

			if (usbIrq & bmSUSPIRQ) // suspend enter
			{
				USBWriteByte(rUSBIRQ, bmSUSPIRQ);/// | bmBUSACTIRQ); // Clear
				// Should arrive here out of reset
				UARTWrite("suspend\n");
				if (devconfig == 1)
					s_suspended = 1;
			}

			if (usbIrq & bmBUSACTIRQ) // suspend exit
			{
				USBWriteByte(rUSBIRQ, bmBUSACTIRQ);
				// USBReadByte(rFNADDR | 0x1);	 // do I need to ack this?
				if (devconfig == 1)
					s_suspended = 0;
			}

			if (usbIrq & bmURESIRQ) // reset enter
			{
				USBWriteByte(rUSBIRQ, bmURESIRQ); // Clear
				UARTWrite("busreset\n");
			}

			if (usbIrq & bmURESDNIRQ) // reset exit
			{
				USBWriteByte(rUSBIRQ, bmURESDNIRQ); // Clear
				UARTWrite("resume\n");
				s_suspended = 0;
				USBEnableIRQs();
			}

			LEDSetState(currLED);

			// Exhaust incoming data buffer and echo to debug and the new com port
			if (s_inputbufferlen && !s_suspended)
			{
				s_outputbufferlen = s_inputbufferlen;
				for (uint8_t i=0; i<s_inputbufferlen; ++i)
					*IO_UARTTX = s_outputbuffer[i] = s_inputbuffer[i];
				s_inputbufferlen = 0;
			}
		}
	}

	return 0;
}
