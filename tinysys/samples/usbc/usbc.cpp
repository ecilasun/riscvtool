#include "basesystem.h"
#include "uart.h"
#include "usbc.h"
#include "leds.h"
#include "usbcdata.h"

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

uint8_t configval;

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

int main(int argc, char *argv[])
{
    UARTWrite("USB-C test\n");

    if (argc>1)
    {
        UARTWrite("Bringing up USB-C\nUsing ISR in ROM\n");
        USBInit(1);
    }
    else
    {
        UARTWrite("Bringing up USB-C\nUsing polling\n");
        USBInit(0);

        // Ordinarily ROM listens to this
        while (1)
        {
            uint32_t currLED = LEDGetState();
            LEDSetState(currLED | 0x8);

            // Initial value of rEPIRQ should be 0x19
	        uint8_t epIrq = USBReadByte(rEPIRQ);
            uint8_t usbIrq = USBReadByte(rUSBIRQ);

            /*UARTWriteHexByte(epIrq);
            UARTWrite(":");
            UARTWriteHexByte(usbIrq);
            UARTWrite(":");
            UARTWriteHexByte(USBGetGPX());
            UARTWrite("\n");*/

            if (epIrq & bmSUDAVIRQ)
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
                    case 0x20: STALL_EP0 break; // class_req
                    case 0x40: STALL_EP0 break; // vendor_req
                    default: STALL_EP0 break;
                }

    		    USBWriteByte(rEPIRQ, bmSUDAVIRQ); // clear SUDAV irq
            }

            if (epIrq & bmIN3BAVIRQ)
            {
                // TODO: asserts out of reset
                USBWriteByte(rEPIRQ, bmIN3BAVIRQ); // Clear
            }

            if ((configval != 0) && (usbIrq & bmSUSPIRQ)) // Suspend
            {
                // Should arrive here out of reset
                USBWriteByte(rUSBIRQ, bmSUSPIRQ | bmBUSACTIRQ); // Clear
            }

            if (usbIrq & bmURESIRQ) // Bus reset
            {
                USBWriteByte(rUSBIRQ, bmURESIRQ); // Clear
            }

            if (usbIrq & bmURESDNIRQ) // Resume
            {
                USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE | bmSUSPIE);
                USBWriteByte(rUSBIRQ, bmURESDNIRQ); // clear URESDN irq
            }

            if (epIrq & bmIN2BAVIRQ)
            {
                // TODO: asserts out of reset
                USBWriteByte(rEPIRQ, bmIN2BAVIRQ); // Clear
            }

            if (epIrq & bmIN0BAVIRQ)
            {
                // TODO: asserts out of reset
                USBWriteByte(rEPIRQ, bmIN0BAVIRQ); // Clear
            }

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
