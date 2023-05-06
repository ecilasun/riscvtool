#include "usbhandler.h"
#include "leds.h"
#include "uart.h"

static uint32_t RWU_enabled = 0;
static uint32_t ep3stall = 0;
static uint32_t usbc_suspended = 0;
static uint32_t usbc_configval = 0;
//static uint32_t usbc_inhibit_send = 0;
//static uint32_t usbc_send3zeros = 0;
//static uint32_t usbc_msgidx = 0;
static uint8_t SUD[8];

const unsigned char strDesc[][64]= { // STRING descriptor 0--Language string
{
    0x04,			// bLength
	0x03,			// bDescriptorType = string
	0x09,0x04		// wLANGID(L/H) = English-United Sates
},
// STRING descriptor 1--Manufacturer ID
{
    12,			// bLength
	0x03,		// bDescriptorType = string
	'M',0,'a',0,'x',0,'i',0,'m',0 // text in Unicode
}, 
// STRING descriptor 2 - Product ID
{	24,			// bLength
	0x03,		// bDescriptorType = string
	'M',0,'A',0,'X',0,'3',0,'4',0,'2',0,'0',0,'E',0,' ',0,
    'E',0,'n',0,'u',0,'m',0,' ',0,'C',0,'o',0,'d',0,'e',0
},

// STRING descriptor 3 - Serial Number ID
{   20,			// bLength
	0x03,		// bDescriptorType = string
	'S',0,'/',0,'N',0,' ',0,'3',0,'4',0,'2',0,'0',0,'E',0,
}};

const unsigned char DD[]=	// DEVICE Descriptor
{   0x12,	       	// bLength = 18d
    0x01,			// bDescriptorType = Device (1)
    0x00,0x01,		// bcdUSB(L/H) USB spec rev (BCD)
	0x00,0x00,0x00, // bDeviceClass, bDeviceSubClass, bDeviceProtocol
	0x40,			// bMaxPacketSize0 EP0 is 64 bytes
	0x6A,0x0B,		// idVendor(L/H)--Maxim is 0B6A
	0x46,0x53,		// idProduct(L/H)--5346
	0x34,0x12,		// bcdDevice--1234
	1,2,3,			// iManufacturer, iProduct, iSerialNumber
	1};				// bNumConfigurations

const unsigned char CD[]=	// CONFIGURATION Descriptor
	{0x09,			// bLength
	0x02,			// bDescriptorType = Config
	0x22,0x00,		// wTotalLength(L/H) = 34 bytes
	0x01,			// bNumInterfaces
	0x01,			// bConfigValue
	0x00,			// iConfiguration
	0xE0,			// bmAttributes. b7=1 b6=self-powered b5=RWU supported
	0x01,			// MaxPower is 2 ma
// INTERFACE Descriptor
	0x09,			// length = 9
	0x04,			// type = IF
	0x00,			// IF #0
	0x00,			// bAlternate Setting
	0x01,			// bNum Endpoints
	0x03,			// bInterfaceClass = HID
	0x00,0x00,		// bInterfaceSubClass, bInterfaceProtocol
	0x00,			// iInterface
// HID Descriptor--It's at CD[18]
	0x09,			// bLength
	0x21,			// bDescriptorType = HID
	0x10,0x01,		// bcdHID(L/H) Rev 1.1
	0x00,			// bCountryCode (none)
	0x01,			// bNumDescriptors (one report descriptor)
	0x22,			// bDescriptorType	(report)
	43,0,           // CD[25]: wDescriptorLength(L/H) (report descriptor size is 43 bytes)
// Endpoint Descriptor
	0x07,			// bLength
	0x05,			// bDescriptorType (Endpoint)
	0x83,			// bEndpointAddress (EP3-IN)		
	0x03,			// bmAttributes	(interrupt)
	64,0,           // wMaxPacketSize (64)
	10};			// bInterval (poll every 10 msec)

const unsigned char RepD[]=   // Report descriptor 
{	0x05,0x01,		// Usage Page (generic desktop)
	0x09,0x06,		// Usage (keyboard)
	0xA1,0x01,		// Collection
	0x05,0x07,		//   Usage Page 7 (keyboard/keypad)
	0x19,0xE0,		//   Usage Minimum = 224
	0x29,0xE7,		//   Usage Maximum = 231
	0x15,0x00,		//   Logical Minimum = 0
	0x25,0x01,		//   Logical Maximum = 1
	0x75,0x01,		//   Report Size = 1
	0x95,0x08,		//   Report Count = 8
	0x81,0x02,		//  Input(Data,Variable,Absolute)
	0x95,0x01,		//   Report Count = 1
	0x75,0x08,		//   Report Size = 8
	0x81,0x01,		//  Input(Constant)
	0x19,0x00,		//   Usage Minimum = 0
	0x29,0x65,		//   Usage Maximum = 101
	0x15,0x00,		//   Logical Minimum = 0,
	0x25,0x65,		//   Logical Maximum = 101
	0x75,0x08,		//   Report Size = 8
	0x95,0x01,		//   Report Count = 1
	0x81,0x00,		//  Input(Data,Variable,Array)
	0xC0};			// End Collection 

// Set all three EP0 stall bits--data stage IN/OUT and status stage
#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23);

// unsigned char CD[]= // CONFIGURATION Descriptor
// {0x09, // bLength
// 0x02, // bDescriptorType = Config
// 0x22,0x00, // wTotalLength(L/H) = 34 bytes
// 0x01, // bNumInterfaces
// 0x01, // bConfigValue
// 0x00, // iConfiguration
// 0xE0, // bmAttributes. b7=1 b6=self-powered b5=RWU supported
// 0x01, // MaxPower is 2 ma

void DoInhibit3()
{
	UARTWrite("DoInhibit3\n");
	/*if (usbc_inhibit_send==0x01)
	{
		USBWriteByte(rEP3INFIFO,0); // send the "keys up" code
		USBWriteByte(rEP3INFIFO,0);
		USBWriteByte(rEP3INFIFO,0);
	}
	else if (usbc_send3zeros==0x01) // precede every keycode with the "no keys" code
	{
		USBWriteByte(rEP3INFIFO,0); // send the "keys up" code
		USBWriteByte(rEP3INFIFO,0);
		USBWriteByte(rEP3INFIFO,0);
		usbc_send3zeros = 0; // next time through this function send the keycode
	}
	else
	{
		usbc_send3zeros = 1;
		USBWriteByte(rEP3INFIFO, Message[usbc_msgidx++]);
		// load the next keystroke (3 bytes)
		USBWriteByte(rEP3INFIFO, Message[usbc_msgidx++]);
		USBWriteByte(rEP3INFIFO, Message[usbc_msgidx++]);
		if(usbc_msgidx >= msglen) // check for message wrap
		{
			usbc_msgidx = 0;
			L0_OFF
			usbc_inhibit_send=1; // send the string once per pushbutton press
		}
		USBWriteByte(rEP3INBC,3); // arm it
	}*/
}

/*void check_for_resume(void)
{
	if(rreg(rUSBIRQ) & bmBUSACTIRQ) // THE HOST RESUMED BUS TRAFFIC
	{
		L2_OFF
		Suspended=0; // no longer suspended
	}
	else if(RWU_enabled) // Only if the host enabled RWU
	{
		if((rreg(rGPIO)&0x40)==0) // See if the Remote Wakeup button was pressed
		{
			L2_OFF // turn off suspend light
			Suspended=0; // no longer suspended
			SETBIT(rUSBCTL,bmSIGRWU)
			// signal RWU
			while ((rreg(rUSBIRQ)&bmRWUDNIRQ)==0) {}; // spin until RWU signaling done
			CLRBIT(rUSBCTL,bmSIGRWU)
			// remove the RESUME signal
			wreg(rUSBIRQ,bmRWUDNIRQ);
			// clear the IRQ
			while((rreg(rGPIO)&0x40)==0) {}; // hang until RWU button released
			wreg(rUSBIRQ,bmBUSACTIRQ);
			// wait for bus traffic -- clear the BUS Active IRQ
			while((rreg(rUSBIRQ) & bmBUSACTIRQ)==0) {}; // & hang here until it's set again...
		}
	}
}*/

void send_descriptor(void)
{
	UARTWrite("send_descriptor\n");
	uint32_t reqlen, sendlen, desclen;
	uint8_t *pDdata; // pointer to ROM Descriptor data to send
	//
	// NOTE This function assumes all descriptors are 64 or fewer bytes and can be sent in a single packet
	//
	desclen = 0;					// check for zero as error condition (no case statements satisfied)
	reqlen = SUD[wLengthL] + 256*SUD[wLengthH];	// 16-bit
		switch (SUD[wValueH])			// wValueH is descriptor type
		{
		case  GD_DEVICE:
				desclen = DD[0];	// descriptor length
				pDdata = (uint8_t*)DD;
				break;	
		case  GD_CONFIGURATION:
				desclen = CD[2];	// Config descriptor includes interface, HID, report and ep descriptors
				pDdata = (uint8_t*)CD;
				break;
		case  GD_STRING:
				desclen = strDesc[SUD[wValueL]][0];   // wValueL=string index, array[0] is the length
				pDdata = (uint8_t*)strDesc[SUD[wValueL]];       // point to first array element
				break;
		case  GD_HID:
				desclen = CD[18];
				pDdata = (uint8_t*)&CD[18];
				break;
		case  GD_REPORT:
				desclen = CD[25];
				pDdata = (uint8_t*)RepD;
			break;
		}	// end switch on descriptor type
	//
	if (desclen!=0)                   // one of the case statements above filled in a value
	{
		sendlen = (reqlen <= desclen) ? reqlen : desclen; // send the smaller of requested and avaiable
		//writebytes(rEP0FIFO, sendlen, pDdata);
		for (uint32_t b=0;b<sendlen;++b)
			USBWriteByte(rEP0FIFO, pDdata[b]);
		//wregAS(rEP0BC, sendlen);   // load EP0BC to arm the EP0-IN transfer & ACKSTAT
		USBWriteByte(rEP0BC|0x01, sendlen); // umm.. bytes!=words, anyone?
	}
	else
		STALL_EP0  // none of the descriptor types match
}

void feature(uint8_t sc)
{
	UARTWrite("feature\n");
	uint8_t mask;
	if((SUD[bmRequestType]==0x02)	// dir=h->p, recipient = ENDPOINT
  		&& (SUD[wValueL]==0x00)		// wValueL is feature selector, 00 is EP Halt
  		&& (SUD[wIndexL]==0x83))	// wIndexL is endpoint number IN3=83
	{
		mask = USBReadByte(rEPSTALLS);   // read existing bits
		if(sc==1)               // set_feature
		{
			mask += bmSTLEP3IN;       // Halt EP3IN
			ep3stall=1;
		}
		else                        // clear_feature
		{
			mask &= ~bmSTLEP3IN;      // UnHalt EP3IN
			ep3stall=0;
			USBWriteByte(rCLRTOGS, bmCTGEP3IN);  // clear the EP3 data toggle
		}
		USBWriteByte(rEPSTALLS, (mask|bmACKSTAT)); // Don't use wregAS for this--directly writing the ACKSTAT bit
	} else if ((SUD[bmRequestType]==0x00)	// dir=h->p, recipient = DEVICE
		&&  (SUD[wValueL]==0x01))	// wValueL is feature selector, 01 is Device_Remote_Wakeup
	{
		RWU_enabled = sc<<1;	// =2 for set, =0 for clear feature. The shift puts it in the get_status bit position.			
		//rregAS(rFNADDR);		// dummy read to set ACKSTAT
		USBReadByte(rFNADDR | 0x1);
	}
	else
		STALL_EP0
}

void get_status(void)
{
	UARTWrite("get_status\n");
	uint8_t testbyte;
	testbyte = SUD[bmRequestType];
	switch(testbyte)	
	{
		case 0x80: 			// directed to DEVICE
			USBWriteByte(rEP0FIFO, (RWU_enabled+1));	// first byte is 000000rs where r=enabled for RWU and s=self-powered.
			USBWriteByte(rEP0FIFO, 0x00);		// second byte is always 0
			//wregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
			USBWriteByte(rEP0BC | 0x1, 2);
		break; 				

		case 0x81: 			// directed to INTERFACE
			USBWriteByte(rEP0FIFO,0x00);		// this one is easy--two zero bytes
			USBWriteByte(rEP0FIFO,0x00);		
			//wregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
			USBWriteByte(rEP0BC | 0x1, 2);
		break; 				

		case 0x82: 			// directed to ENDPOINT
			if(SUD[wIndexL]==0x83)		// We only reported ep3, so it's the only one the host can stall IN3=83
			{
				USBWriteByte(rEP0FIFO,ep3stall);	// first byte is 0000000h where h is the halt (stall) bit
				USBWriteByte(rEP0FIFO,0x00);		// second byte is always 0
				//wregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
				USBWriteByte(rEP0BC | 0x1, 2);
			}
			else
				STALL_EP0		// Host tried to stall an invalid endpoint (not 3)
		break;

		default:
			STALL_EP0		// don't recognize the request
	}
}

void set_interface(void)	// All we accept are Interface=0 and AlternateSetting=0, otherwise send STALL
{
	//uint8_t dumval;
	if((SUD[wValueL]==0)					// wValueL=Alternate Setting index
  		&&(SUD[wIndexL]==0))				// wIndexL=Interface index
  		/*dumval =*/ USBReadByte(rFNADDR | 0x1);	// dummy read to set the ACKSTAT bit
	else
		STALL_EP0
}


void get_interface(void)	// Check for Interface=0, always report AlternateSetting=0
{
	UARTWrite("get_interface\n");
	if(SUD[wIndexL]==0)		// wIndexL=Interface index
	{
		USBWriteByte(rEP0FIFO, 0);		// AS=0
		//wregAS(rEP0BC,1);		// send one byte, ACKSTAT
		USBWriteByte(rEP0BC | 0x1, 1);
	}
	else
		STALL_EP0
}

void get_configuration(void)
{
	UARTWrite("get_configuration\n");
	USBWriteByte(rEP0FIFO, usbc_configval);         // Send the config value
	//wregAS(rEP0BC,1);   
	USBWriteByte(rEP0BC | 0x1, 1);
}

void set_configuration(void)
{
	UARTWrite("set_configuration\n");
	usbc_configval =SUD[wValueL];           // Store the config value
	if(usbc_configval != 0)                // If we are configured, 
	{
  		//SETBIT(rUSBIEN,bmSUSPIE);       // start looking for SUSPEND interrupts
		USBWriteByte(rUSBIEN, USBReadByte(rUSBIEN) | bmSUSPIE);
	}
	//rregAS(rFNADDR);                  // dummy read to set the ACKSTAT bit
	USBReadByte(rFNADDR | 0x1);
}

void std_request()
{
	UARTWrite("std_request\n");
	switch(SUD[bRequest])
	{
		case SR_GET_DESCRIPTOR: send_descriptor(); break;
		case SR_SET_FEATURE: feature(1); break;
		case SR_CLEAR_FEATURE: feature(0); break;
		case SR_GET_STATUS: get_status(); break;
		case SR_SET_INTERFACE: set_interface(); break;
		case SR_GET_INTERFACE: get_interface(); break;
		case SR_GET_CONFIGURATION: get_configuration(); break;
		case SR_SET_CONFIGURATION: set_configuration(); break;
		case SR_SET_ADDRESS: /*rregAS(rFNADDR)*/ USBReadByte(rFNADDR | 0x1); break; // discard return value
		default: STALL_EP0 break;
	}
}

void class_request()
{
	UARTWrite("class_request\n");
	STALL_EP0
}

void vendor_request()
{
	UARTWrite("vendor_request\n");
	STALL_EP0
}

void DoSetup()
{
	UARTWrite("DoSetup:");
	USBReadBytes(rSUDFIFO, 8, SUD, 0xFF);

	// if no packet, STALL_EP0

	UARTWriteHexByte(SUD[0]);
	UARTWriteHexByte(SUD[1]);
	UARTWriteHexByte(SUD[2]);
	UARTWriteHexByte(SUD[3]);
	UARTWriteHexByte(SUD[4]);
	UARTWriteHexByte(SUD[5]);
	UARTWriteHexByte(SUD[6]);
	UARTWriteHexByte(SUD[7]);
	UARTWrite("\n");

	switch(SUD[bmRequestType]&0x60)
	{
		case 0x00: std_request(); break;
		case 0x20: class_request(); break;
		case 0x40: vendor_request(); break;
		default: STALL_EP0 break;
	}
}

void HandleUSBC()
{
	uint32_t currLED = LEDGetState();
	currLED &= ~0x4;

	uint32_t epIrq = USBReadByte(rEPIRQ);
	uint32_t usbIrq = USBReadByte(rUSBIRQ);
	if (epIrq == 0xFF && usbIrq == 0xFF)
		return;

	UARTWriteHexByte(epIrq);
	UARTWrite(":");
	UARTWriteHexByte(usbIrq);
	UARTWrite(" ");

	if (usbIrq & bmURESDNIRQ)
	{
		UARTWrite("resetoff\n");
		currLED &= ~0x8;
		USBWriteByte(rUSBIRQ, bmURESDNIRQ);
		usbc_suspended = 0;
		USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE); // enable irqs
	}
	else if (epIrq & bmSUDAVIRQ)
	{
		DoSetup();
		USBWriteByte(rEPIRQ, bmSUDAVIRQ); // clear SUDAV irq
	}
	else if (usbIrq & bmURESIRQ)
	{
		UARTWrite("busreset\n");
		currLED |= 0x8;
		USBWriteByte(rUSBIRQ, bmURESIRQ);
	}
	else if (epIrq & bmIN3BAVIRQ)
	{
		currLED |= 0x4;
		DoInhibit3();
	}
	else if ((usbc_configval != 0) && usbIrq & bmSUSPIRQ) // host suspend bus for 3ms
	{
		UARTWrite("suspend3ms\n");
		USBWriteByte(rUSBIRQ, bmSUDAVIRQ|bmBUSACTIRQ);
		usbc_suspended = 1;
	}

	LEDSetState(currLED);
}
