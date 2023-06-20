#include "usbc.h"
#include "basesystem.h"
#include "uart.h"
#include <string.h>

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) DEVICE_USBC; // Receive fifo
volatile uint32_t *IO_USBCSTA = (volatile uint32_t* ) (DEVICE_USBC+4); // Output FIFO state

static uint32_t statusF = 0;
static uint32_t sparebyte = 0;
static struct SUSBContext s_usb;

#define ASSERT_CS *IO_USBCTRX = 0x100;
#define RESET_CS *IO_USBCTRX = 0x101;

struct SUSBContext *USBGetContext()
{
    return &s_usb;
}

void USBFlushOutputFIFO()
{
    while ((*IO_USBCSTA)&0x1) {}
}

uint8_t USBGetGPX()
{
    return (*IO_USBCSTA)&0x2;
}

uint8_t USBReadByte(uint8_t command)
{
    ASSERT_CS
    *IO_USBCTRX = command;   // -> status in read FIFO
    statusF = *IO_USBCTRX;   // unused
    *IO_USBCTRX = 0;         // -> read value in read FIFO
    sparebyte = *IO_USBCTRX; // output value
    RESET_CS

    return sparebyte;
}

void USBWriteByte(uint8_t command, uint8_t data)
{
    ASSERT_CS
    *IO_USBCTRX = command | 0x02; // -> zero in read FIFO
    statusF = *IO_USBCTRX;        // unused
    *IO_USBCTRX = data;           // -> zero in read FIFO
    sparebyte = *IO_USBCTRX;      // unused
    RESET_CS
}

int USBReadBytes(uint8_t command, uint8_t length, uint8_t *buffer)
{
    ASSERT_CS
    *IO_USBCTRX = command;   // -> status in read FIFO
    statusF = *IO_USBCTRX;   // unused
    //*IO_USBCTRX = 0;         // -> read value in read FIFO
    //sparebyte = *IO_USBCTRX; // output value

    for (int i=0; i<length; i++)
    {
        *IO_USBCTRX = 0;          // send one dummy byte per input desired
        buffer[i] = *IO_USBCTRX;  // store data byte
    }
    RESET_CS

    return 0;
}

void USBWriteBytes(uint8_t command, uint8_t length, uint8_t *buffer)
{
    ASSERT_CS
    *IO_USBCTRX = command | 0x02;   // -> status in read FIFO
    statusF = *IO_USBCTRX;          // unused
    //*IO_USBCTRX = 0;         // -> read value in read FIFO
    //sparebyte = *IO_USBCTRX; // output value

    for (int i=0; i<length; i++)
    {
        *IO_USBCTRX = buffer[i];  // send one dummy byte per input desired
        sparebyte = *IO_USBCTRX;  // unused
    }
    RESET_CS
}

void USBCtlReset()
{
    // Reset
    USBWriteByte(rUSBCTL, bmCHIPRES);    // reset the MAX3420E 
    USBWriteByte(rUSBCTL, 0);            // remove the reset

    USBFlushOutputFIFO();
    E32Sleep(3*ONE_MILLISECOND_IN_TICKS);

    // Wait for oscillator OK interrupt
    UARTWrite("Waiting for 12MHz oscillator...");
    uint8_t rd = 0;
    while ((rd & bmOSCOKIRQ) == 0)
    {
        rd = USBReadByte(rUSBIRQ);
        E32Sleep(3*ONE_MILLISECOND_IN_TICKS);
    }
    USBWriteByte(rUSBIRQ, bmOSCOKIRQ);

    USBWriteByte(rGPIO, 0x0);            // No GPIO output

    if (rd&bmBUSACTIRQ)
        UARTWrite("usb bus active,\n");
    UARTWrite("OK\n");
}

#ifdef __cplusplus
char16_t vendorname[] = u"ENGIN";
char16_t devicename[] = u"tinysys usb serial";
char16_t deviceserial[] = u"S/N 00001";
#else
char vendorname[] = {'E',0,'N',0,'G',0,'I',0,'N',0};
char devicename[] = {'t',0,'i',0,'n',0,'y',0,'s',0,'y',0,'s',0,' ',0,'u',0,'s',0,'b',0,' ',0,'s',0,'e',0,'r',0,'i',0,'a',0,'l',0};
char deviceserial[] = {'S',0,'/',0,'N',0,' ',0,'0',0,'0',0,'0',0,'0',0,'1',0};
#endif

void USBMakeCDCDescriptors(struct SUSBContext *ctx)
{
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/network/device-descriptor
    // https://www.usb.org/sites/default/files/CDC_EEM10.pdf
    // https://beyondlogic.org/usbnutshell/usb6.shtml

    // Device
    ctx->device.bLength = sizeof(struct USBDeviceDescriptor); // 18
    ctx->device.bDescriptorType = USBDesc_Device;
    ctx->device.bcdUSB = 0x0101;
    ctx->device.bDeviceClass = USBClass_CDCControl;
    ctx->device.bDeviceSubClass = 0x0;
    ctx->device.bDeviceProtocol = 0x0;
    ctx->device.bMaxPacketSizeEP0 = 64;
    ctx->device.idVendor = 0xFFFF;
    ctx->device.idProduct = 0x0001;
    ctx->device.bcdDevice = 0x1234;
    ctx->device.iManufacturer = 1;
    ctx->device.iProduct = 2;
    ctx->device.iSerialNumber = 3;
    ctx->device.bNumConfigurations =1;

    // Configuration
    ctx->config.bLength = sizeof(struct USBConfigurationDescriptor); // 9
    ctx->config.bDescriptorType = USBDesc_Configuration;
    ctx->config.wTotalLength = 0x0030; // 48 bytes; includes config, interface and endpoints (not the strings)
    ctx->config.bNumInterfaces = 2;
    ctx->config.bConfigurationValue = 1;
    ctx->config.iConfiguration = 0;
    ctx->config.bmAttributes = 0x80; // Bus powered
    ctx->config.MaxPower = 0xFA; // 500 mA

    // Control Interface
    ctx->control.bLength = sizeof(struct USBInterfaceDescriptor); // 9
    ctx->control.bDescriptorType = USBDesc_Interface;
    ctx->control.bInterfaceNumber = 0;   // Interface #0
    ctx->control.bAlternateSetting = 0;
    ctx->control.bNumEndpoints = 1;      // 1 endpoint (control)
    ctx->control.bInterfaceClass = USBClass_CDCControl;
    ctx->control.bInterfaceSubClass = 0x02; // Abstract
    ctx->control.bInterfaceProtocol = 0xFF; // Vendor specific
    ctx->control.iInterface = 0;

    // Control Notification
    ctx->notification.bLength = sizeof(struct USBEndpointDescriptor); // 7
    ctx->notification.bDescriptorType = USBDesc_Endpoint;
    ctx->notification.bEndpointAddress = 0x81;
    ctx->notification.bmAttributes = 0x03; // Interrupt endpoint
    ctx->notification.wMaxPacketSize = 64;
    ctx->notification.bInterval = 1;       // Every millisecond

    // Data Interface
    ctx->data.bLength = sizeof(struct USBInterfaceDescriptor); // 9
    ctx->data.bDescriptorType = USBDesc_Interface;
    ctx->data.bInterfaceNumber = 1;   // Interface #1
    ctx->data.bAlternateSetting = 0;
    ctx->data.bNumEndpoints = 2;      // 2 endpoints (data in / data out)
    ctx->data.bInterfaceClass = USBClass_CDCData;
    ctx->data.bInterfaceSubClass = 0x00;
    ctx->data.bInterfaceProtocol = 0x00;
    ctx->data.iInterface = 0;

    // Data in
    ctx->input.bLength = sizeof(struct USBEndpointDescriptor); // 7
    ctx->input.bDescriptorType = USBDesc_Endpoint;
    ctx->input.bEndpointAddress = 0x82;   // EP2 in
    ctx->input.bmAttributes = 0x02;       // Bulk endpoint
    ctx->input.wMaxPacketSize = 64;
    ctx->input.bInterval = 0;

    // Data out
    ctx->output.bLength = sizeof(struct USBEndpointDescriptor); // 7
    ctx->output.bDescriptorType = USBDesc_Endpoint;
    ctx->output.bEndpointAddress = 0x03;  // EP3 out
    ctx->output.bmAttributes = 0x02;      // Bulk endpoint
    ctx->output.wMaxPacketSize = 64;
    ctx->output.bInterval = 0;

    // Strings
    ctx->strings[0].bLength = sizeof(struct USBStringLanguageDescriptor); // 4
    ctx->strings[0].bDescriptorType = USBDesc_String;
#ifdef __cplusplus
    ctx->strings[0].bString[0] = 0x0409; // English-United Sates
#else
    ctx->strings[0].bString[0] = 0x09; // English-United Sates
    ctx->strings[0].bString[1] = 0x04;
#endif
    ctx->strings[1].bLength = sizeof(struct USBCommonDescriptor) + 5*2; // 12
    ctx->strings[1].bDescriptorType = USBDesc_String;
    __builtin_memcpy(ctx->strings[1].bString, vendorname, 10);
    ctx->strings[2].bLength = sizeof(struct USBCommonDescriptor) + 18*2; // 38
    ctx->strings[2].bDescriptorType = USBDesc_String;
    __builtin_memcpy(ctx->strings[2].bString, devicename, 36);
    ctx->strings[3].bLength = sizeof(struct USBCommonDescriptor) + 9*2; // 20
    ctx->strings[3].bDescriptorType = USBDesc_String;
    __builtin_memcpy(ctx->strings[3].bString, deviceserial, 9);
}

void USBInit(uint32_t enableInterrupts)
{
    // Generate descriptor table
    USBMakeCDCDescriptors(&s_usb);

    USBWriteByte(rPINCTL, bmFDUPSPI | bmINTLEVEL | gpxSOF); // MAX3420: SPI=full-duplex

    USBCtlReset();

    if (enableInterrupts)
    {
        // Enable IRQs
        USBWriteByte(rEPIEN, bmSUDAVIE | bmIN3BAVIE);
        // bmSUSPIE is to be enabled after the device initializes
        USBWriteByte(rUSBIEN, bmURESIE | bmURESDNIE | bmSUSPIE);

        // Enable interrupt generation via INT pin
        USBWriteByte(rCPUCTL, bmIE);
    }

    USBWriteByte(rUSBCTL, bmCONNECT | bmVBGATE);
}
