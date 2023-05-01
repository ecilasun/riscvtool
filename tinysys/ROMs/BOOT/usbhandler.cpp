#include "usbhandler.h"
#include "leds.h"

static uint32_t usbc_suspended = 0;
static uint32_t usbc_configval = 0;
static uint32_t usbc_inhibit_send = 0;
static uint32_t usbc_send3zeros = 0;
static uint32_t usbc_msgidx = 0;
static uint8_t SUD[8];

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

void std_request()
{
	/*switch(SUD[bRequest])
	{
		case SR_GET_DESCRIPTOR:
		case SR_SET_FEATURE:
		case SR_CLEAR_FEATURE:
		case SR_GET_STATUS:
		case SR_SET_INTERFACE:
		case SR_GET_INTERFACE:
		case SR_GET_CONFIGURATION:
		case SR_SET_CONFIGURATION:
		case SR_SET_ADDRESS:
		default: STALL_EP0
	}*/
}

void class_request()
{
	// Stub
}

void vendor_request()
{
	// Stub
}

void DoSetup()
{
	//USBReadBytes(rSUDFIFO, 8, SUD);
	SUD[0] = USBReadByte(rSUDFIFO);
	SUD[1] = USBReadByte(rSUDFIFO);
	SUD[2] = USBReadByte(rSUDFIFO);
	SUD[3] = USBReadByte(rSUDFIFO);
	SUD[4] = USBReadByte(rSUDFIFO);
	SUD[5] = USBReadByte(rSUDFIFO);
	SUD[6] = USBReadByte(rSUDFIFO);
	SUD[7] = USBReadByte(rSUDFIFO);
	switch(SUD[bmRequestType]&0x60)
	{
		case 0x00: std_request();
		case 0x20: class_request();
		case 0x40: vendor_request();
		default: STALL_EP0
	}
}

void HandleUSBC()
{
	uint32_t currLED = LEDGetState();

	uint32_t itest1 = USBReadByte(rEPIRQ);
	uint32_t itest2 = USBReadByte(rUSBIRQ);

	if (itest1 & bmSUDAVIRQ)
	{
		USBWriteByte(rEPIRQ, bmSUDAVIRQ); // clear SUDAV irq
		DoSetup();
	}

	if (itest1 & bmIN3BAVIRQ)
	{
		currLED |= 0x4;
		DoInhibit3();
	}

	if ((usbc_configval != 0) && itest2 & bmSUSPIRQ) // host suspend bus for 3ms
	{
		USBWriteByte(rUSBIRQ, bmSUDAVIRQ|bmBUSACTIRQ);
		usbc_suspended = 1;
	}

	if (itest2 & bmURESIRQ) // bus reset
	{
		currLED |= 0x8;
		USBWriteByte(rUSBIRQ, bmURESIRQ);
	}

	if (itest2 & bmURESDNIRQ) // bus reset light off
	{
		currLED &= ~0x8;
		USBWriteByte(rUSBIRQ, bmURESDNIRQ);
		usbc_suspended = 0;
		USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE); // enable irqs
	}

	LEDSetState(currLED);
}
