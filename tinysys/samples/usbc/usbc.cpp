#include "basesystem.h"
#include "uart.h"
#include "usbc.h"
#include "leds.h"
#include "usbcdata.h"

#define STALL_EP0 USBWriteByte(rEPSTALLS, 0x23); 
#define SETBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)|val));
#define CLRBIT(reg,val) USBWriteByte(reg, (USBReadByte(reg)&~val));

uint8_t configval;

/*void get_status(void)
{
    switch(SUD[bmRequestType])	
	{
	    case 0x80: 			// directed to DEVICE
            USBWriteByte(rEP0FIFO, (RWU_enabled+1)); // first byte is 000000rs where r=enabled for RWU and s=self-powered.
            USBWriteByte(rEP0FIFO, 0x00);		     // second byte is always 0
            USBWriteByte(rEP0BC | 0x01, 2);		     // load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
		break; 				
	    case 0x81: 			// directed to INTERFACE
            wreg(rEP0FIFO,0x00);		// this one is easy--two zero bytes
            wreg(rEP0FIFO,0x00);		
            wregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
		break; 				
	    case 0x82: 			// directed to ENDPOINT
            if(SUD[wIndexL]==0x83)		// We only reported ep3, so it's the only one the host can stall IN3=83
            {
                wreg(rEP0FIFO,ep3stall);	// first byte is 0000000h where h is the halt (stall) bit
                wreg(rEP0FIFO,0x00);		// second byte is always 0
                wregAS(rEP0BC,2); 		// load byte count, arm the IN transfer, ACK the status stage of the CTL transfer
                break;
            }
		else
        STALL_EP0		// Host tried to stall an invalid endpoint (not 3)				
	    default:      STALL_EP0		// don't recognize the request
	}
}*/

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

int main(int argc, char *argv[])
{
    UARTWrite("USB-C test\n");

    if (argc>1)
    {
        UARTWrite("Bringing up USB-C\n");
        USBInit();
    }
    else
    {
        UARTWrite("USB-C SPI r/w development test\n");

        // Half duplex without this
        USBWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF); // MAX3420: SPI=full-duplex 
        USBWriteByte(rUSBCTL, bmCHIPRES);                       // reset the MAX3420E 
        USBWriteByte(rUSBCTL, 0);                               // remove the reset

        USBFlushOutputFIFO();
        E32Sleep(3*ONE_MILLISECOND_IN_TICKS);

        // Wait for oscillator OK interrupt
        UARTWrite("Waiting for oscillator...\n");
        uint8_t rd = 0;
        while ((rd & bmOSCOKIRQ) == 0)
        {
            rd = USBReadByte(rUSBIRQ);
            UARTWriteHexByte(rd);
            E32Sleep(3*ONE_MILLISECOND_IN_TICKS);
        }
        USBWriteByte(rUSBIRQ, bmOSCOKIRQ);
        UARTWrite("\nMAX3420e: oscillator OK\n");
        if (rd&bmBUSACTIRQ)
            UARTWrite("MAX3420e: bus active\n");

        USBWriteByte(rGPIO, 0x0);                    // set all GPIO out to zero
        USBWriteByte(rUSBCTL, bmCONNECT | bmVBGATE); // connect

        //USBWriteByte(rCPUCTL, bmIE); // NOTE: DO NOT enable interrupts yet, this would redirect all to ROM

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

        while (1)
        {
            uint32_t currLED = LEDGetState();
            // Clear top 2 bits
            currLED &= 0x3;

	        uint8_t epIrq = USBReadByte(rEPIRQ);
            uint8_t usbIrq = USBReadByte(rUSBIRQ);

        	if (usbIrq & bmURESDNIRQ) // Woke up?
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
                    case 0x20: STALL_EP0 break; // class_req
                    case 0x40: STALL_EP0 break; // vendor_req
                    default: STALL_EP0 break;
                }
                currLED |= 0x4;
            }

            if (usbIrq & bmURESIRQ)
            {
                // Post-reset
                USBWriteByte(rUSBIRQ, bmURESIRQ); // clear URES irq
            }

            if (epIrq & bmIN3BAVIRQ)
            {
                // TODO:
            }

            if (usbIrq & bmSUSPIRQ)
            {
                // Suspend
                USBWriteByte(rUSBIRQ, bmSUDAVIRQ | bmBUSACTIRQ);  // clear SUDAV/BUSACT irqs
            }

            LEDSetState(currLED);
        }
    }

    return 0;
}
