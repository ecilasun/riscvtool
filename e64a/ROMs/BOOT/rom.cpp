// Bootloader

#include "rvcrt0.h"

#include "core.h"

volatile uint32_t *uartrx = (volatile uint32_t*)0x80000000;
volatile uint32_t *uarttx = (volatile uint32_t*)0x80000004;
volatile uint32_t *uartstat = (volatile uint32_t*)0x80000008; // 31:8=Rsrv 7=ParityErr 6=FrameErr 5=OverrunErr 4=IntrEna 3=TxFull 2=TxEmpty 1=RxFull 0:RxDataValid
volatile uint32_t *uartctl = (volatile uint32_t*)0x8000000C; // 31:5=Rsrv 4=EnableInt 3:2=Rsrv 1=ResetRxFIFO 0=ResetTxFIFO

int main()
{
	*uarttx = 'H';
	*uarttx = 'e';
	*uarttx = 'l';
	*uarttx = 'l';
	*uarttx = 'o';
	*uarttx = '.';
	*uarttx = '\n';

	while(1)
	{
		// TODO:
	}

	return 0;
}
