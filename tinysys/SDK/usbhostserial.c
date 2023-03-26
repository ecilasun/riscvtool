#include "uart.h"

#include <math.h> // For abs()

// Status register bits
// uartstateregister: {30'd0, fifoFull, inFifohasData};

volatile uint32_t *IO_USBHOSTRX     = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBHOSTTX     = (volatile uint32_t* ) 0x80007004; // Transmit fifo
volatile uint32_t *IO_USBHOSTStatus = (volatile uint32_t* ) 0x80007008; // Status register
//volatile uint32_t *IO_USBHOSTCtl    = (volatile uint32_t* ) 0x8000700C; // Control register

int USBInputFifoHasData()
{
	// bit0: RX FIFO has valid data
	return ((*IO_USBHOSTStatus)&0x00000001); // inFifohasData
}

void USBDrainInput()
{
	while (USBInputFifoHasData()) { asm volatile("nop;"); }
}

uint8_t USBRead()
{
	return (uint8_t)(*IO_USBHOSTRX);
}

void USBWrite(const char *_message)
{
	// Emit all characters from the input string
	int i = 0;
	while (_message[i]!=0)
		*IO_USBHOSTTX = (uint32_t)_message[i++];
}
