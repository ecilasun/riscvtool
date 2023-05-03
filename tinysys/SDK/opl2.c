#include "opl2.h"
#include "basesystem.h"

volatile uint8_t *IO_OPL2REG = (volatile uint8_t*)0x80009000;
volatile uint8_t *IO_OPL2VAL = (volatile uint8_t*)0x80009004;

void OPL2WriteReg(uint8_t reg)
{
    *IO_OPL2REG = reg;
	E32Sleep(200);	// Need to give time for internal processing
}

void OPL2WriteVal(uint8_t val)
{
	*IO_OPL2VAL = val;
	E32Sleep(200);	// Need to give time for internal processing
}

void OPL2Stop()
{
	// TODO: Determine the series of OPL2 commands to write to stop all channels from playing
}