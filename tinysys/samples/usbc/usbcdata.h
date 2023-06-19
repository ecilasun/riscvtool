// https://learn.microsoft.com/en-us/windows-hardware/drivers/network/device-descriptor
// https://www.usb.org/sites/default/files/CDC_EEM10.pdf

// Device descriptor
unsigned char DD[]=
{	0x12,	       	// bLength
    0x01,			// bDescriptorType = Device (1)
    0x01,0x01,		// bcdUSB(L/H) USB spec rev (BCD) - 1.1
	0x02,0x00,0x00, // bDeviceClass (CDC), bDeviceSubClass, bDeviceProtocol
	0x40,			// bMaxPacketSize0 EP0 is 64 bytes for full speed
	0xFF,0xFF,		// idVendor(L/H)--Maxim is 0B6A
	0x01,0x00,		// idProduct(L/H)--5346
	0x34,0x12,		// bcdDevice--1234
	1,2,3,			// iManufacturer, iProduct, iSerialNumber - string indices
	1};				// bNumConfigurations

// Configuration Descriptor
unsigned char CD[]=
{	0x09,		// bLength
	0x02,		// bDescriptorType = Config
	0x3E,0x00,	// wTotalLength(L/H) - 62 bytes
	0x02,		// bNumInterfaces (2 total)
	0x01,		// bConfigValue
	0x00,		// iConfiguration - unused
	0x80,		// bmAttributes. b7=bus-powered
	0xFA,		// MaxPower is 500 ma (we pull all power for the entire board)
// Interface Descriptor - CCI
	0x09,		// length = 9
	0x04,		// type = IF
	0x00,		// IF #0
	0x00,		// bAlternate Setting
	0x01,		// bNum Endpoints
	0x02,		// bInterfaceClass = Communication Class
	0x02,0xFF,	// bInterfaceSubClass (abstract), bInterfaceProtocol (vendor specific)
	0x00,		// iInterface - unused
// Notification Endpoint Descriptor
	0x07,		// bLength
	0x05,		// bDescriptorType - Endpoint
	0x81,		// bEndpointAddress - EP1 IN
	0x03,		// bmAttributes - Interrupt endpoint
	0x40,0x00,	// wMaxPacketSize - 64 byte max
	0x01,		// bInterval - polling interval
// Interface Descriptor - DCI
	0x09,		// bLength
	0x04,		// bDescriptorType - Interface descriptor
	0x01,		// bInterfaceNumber - 1
	0x00,		// bAlternateSetting
	0x02,		// bNumEndpoints - 2 endpoints
	0x0A,		// bInterfaceClas - Data class
	0x00,		// bInterfaceSubclass
	0x00,		// bInterfaceProtocol
	0x00,		// iInterface
// Endpoint Descriptor - Data In
	0x07,		// bLength
	0x05,		// bDescriptorType - Endpoint
	0x82,		// bEndpointAddress - EP2 In
	0x02,		// bmAttributes	- Bulk endpoint
	64,0,		// wMaxPacketSize - 64 bytes
	0x0,		// bInterval - unused
// Endpoint Descriptor - Data Out
	0x07,		// bLength
	0x05,		// bDescriptorType - Endpoint
	0x03,		// bEndpointAddress - EP3 Out
	0x02,		// bmAttributes	- Bulk endpoint
	64,0,		// wMaxPacketSize - 64 bytes
	0x0,		// bInterval - unused
};

// STRING descriptors. An array of string arrays of 64 bytes each

unsigned char strDesc[][64]= {
// STRING descriptor 0--Language string
{
    4,			// bLength
	0x03,		// bDescriptorType = string
	0x09,0x04	// wLANGID(L/H) = English-United Sates
},
// STRING descriptor 1--Manufacturer ID
{
    12,			// bLength
	0x03,		// bDescriptorType = string
	'E',0,
	'N',0,
	'G',0,
	'I',0,
	'N',0 // text in Unicode
},

// STRING descriptor 2 - Product ID
{	38,			// bLength
	0x03,		// bDescriptorType = string
	't',0,
	'i',0,
	'n',0,
	'y',0,
	's',0,
	'y',0,
	's',0,
	' ',0,
	'u',0,
	's',0,
	'b',0,
	' ',0,
	's',0,
	'e',0,
	'r',0,
	'i',0,
	'a',0,
	'l',0
},

// STRING descriptor 3 - Serial Number ID
{   20,				// bLength
	0x03,			// bDescriptorType = string
	'S',0,
	'/',0,
	'N',0,
	' ',0,
	'0',0,
	'0',0,
	'0',0,
	'0',0,
    '1',0,
}};
