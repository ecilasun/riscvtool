#include "basesystem.h"
#include "uart.h"
#include "usbhostserial.h"
#include <stdio.h>

// Based on https://github.com/MatzElectronics/CH559sdccUSBHost/tree/master/USBhostCH559propellerExample

#define MSG_TYPE_CONNECTED      0x01
#define MSG_TYPE_DISCONNECTED   0x02
#define MSG_TYPE_ERROR          0x03
#define MSG_TYPE_DEVICE_POLL    0x04
#define MSG_TYPE_DEVICE_STRING  0x05
#define MSG_TYPE_DEVICE_INFO    0x06
#define MSG_TYPE_HID_INFO       0x07
#define MSG_TYPE_STARTUP        0x08

unsigned char uartRxBuff[1024];
int  rxPos = 0;
int  cmdLength = 0;
unsigned char  cmdType = 0;

const char *deviceType[] = {"UNKNOWN", "POINTER", "MOUSE", "RESERVED", "JOYSTICK", "GAMEPAD", "KEYBOARD", "KEYPAD", "MULTI_AXIS", "SYSTEM"};

void filterCommand(int buffLength, unsigned char *msgbuffer);

int main()
{
    UARTWrite("USB serial interface test\r\n");

	while(1)
	{
		while(1)
		{
			if (USBInputFifoHasData())
			{
				uartRxBuff[rxPos] = USBRead();
				if (rxPos == 0 && uartRxBuff[rxPos] == 0xFE)
				{
					UARTWrite("len(");
					cmdType = 1;
				}
				else if (rxPos == 1 && cmdType == 1)
				{
					cmdLength = uartRxBuff[rxPos];
				}
				else if (rxPos == 2 && cmdType == 1)
				{
					cmdLength += (uartRxBuff[rxPos] << 8);
					UARTWriteDecimal(cmdLength);
					UARTWrite(");");
				}
				else if (cmdType == 0 && uartRxBuff[rxPos] == '\n')
				{
					UARTWrite("?;");
					// No command received
					rxPos = 0;
					cmdType = 0;
					break;
				}

				if (((rxPos > 0) && (rxPos == cmdLength + 11) && (cmdType != 0)) || (rxPos > 1024))
				{
					UARTWrite("!\r\n");
					filterCommand(cmdLength, uartRxBuff);
					rxPos = 0;
					cmdType = 0;
					break;
				}
				else
				{
					if (rxPos >2)
						UARTWriteHexByte(uartRxBuff[rxPos]);
					rxPos++;
				}
			}
		}
		rxPos = 0;
	}
}

void filterCommand(int buffLength, unsigned char *msgbuffer)
{
	int cmdLength = buffLength;
	unsigned char msgType = msgbuffer[3];
	unsigned char devType = msgbuffer[4];
	unsigned char device = msgbuffer[5];
	//unsigned char endpoint = msgbuffer[6];
	//unsigned char idVendorL = msgbuffer[7];
	//unsigned char idVendorH = msgbuffer[8];
	//unsigned char idProductL = msgbuffer[9];
	//unsigned char idProductH = msgbuffer[10];
	switch (msgType)
	{
		case MSG_TYPE_CONNECTED:
			printf("Connection port %d\r", device);
		break;
		case MSG_TYPE_DISCONNECTED:
			printf("Disconnection port %d\r", device);
		break;
		case MSG_TYPE_ERROR:
			printf("%s error port %d\r", deviceType[devType], device);
		break;
		case MSG_TYPE_DEVICE_POLL:
			printf("%s Data port: %d, %d bytes, ID: ", deviceType[devType], device, cmdLength);
			for (int j = 0; j < 4; j++)
			{
				printf("%02x", msgbuffer[j + 7]);
			}
			printf(", ");
			for (int j = 0; j < cmdLength; j++)
			{
				printf("%02x", msgbuffer[j + 11]);
			}
			printf("\r");

			// Mouse
			if (devType == 2)
			{
        		/*x += (int16_t)((uint8_t)msgbuffer[11 + 2] + ((uint8_t)msgbuffer[11 + 3] << 8));
        		y += (int16_t)((uint8_t)msgbuffer[11 + 4] + ((uint8_t)msgbuffer[11 + 5] << 8));
				if (msgbuffer[11 + 1] & 1)
				{
					button = 1;
				}
				else
					button = 0;
				if (msgbuffer[11 + 1] & 2)
				{
					button1 = 1;
				}
				else
					button1 = 0;*/
			}

			// Keyboard
			if (devType == 6)
			{
				/*if (msgbuffer[11 + 0] == 2)
					shift = 1;
				else
				shift = 0;
				newkey = msgbuffer[11 + 2];*/
			}

		break;
		case MSG_TYPE_DEVICE_STRING:
			printf("String from port %d: ", devType);
			for (int j = 0; j < cmdLength; j++)
			{
				printf("%c", msgbuffer[j + 11]);
			}
			printf("\r");
		break;
		case MSG_TYPE_DEVICE_INFO:
			printf("Info from port %d: ", device);
			for (int j = 0; j < cmdLength; j++)
			{
				printf("0x%02x ", msgbuffer[j + 11]);
			}
			printf("\r");
		break;
		case MSG_TYPE_HID_INFO:
			printf("HID info from port %d: ", device);
			for (int j = 0; j < cmdLength; j++)
			{
				printf("0x%02x ", msgbuffer[j + 11]);
			}
			printf("\r");
		break;
		case MSG_TYPE_STARTUP:
			printf("USB host ready\r");
		break;
	}
}
