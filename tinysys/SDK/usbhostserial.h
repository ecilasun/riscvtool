#pragma once

#include <inttypes.h>

extern volatile uint32_t *IO_USBHOSTRX;
extern volatile uint32_t *IO_USBHOSTTX;
extern volatile uint32_t *IO_USBHOSTStatus;
extern volatile uint32_t *IO_USBHOSTCtl;

void USBDrainInput();
int USBInputFifoHasData();
void USBWrite(const char *_message);
uint8_t USBRead();
