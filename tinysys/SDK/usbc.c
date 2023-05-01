#include "usbc.h"

volatile uint32_t *IO_USBCTRX = (volatile uint32_t* ) 0x80007000; // Receive fifo
volatile uint32_t *IO_USBCCSn = (volatile uint32_t* ) 0x80007004; // CSn wire
