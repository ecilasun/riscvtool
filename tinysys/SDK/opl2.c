#include "opl2.h"

volatile uint8_t *IO_OPL2REG = (volatile uint8_t*)0x80009000;
volatile uint8_t *IO_OPL2VAL = (volatile uint8_t*)0x80009004;

void OPL2WriteReg(uint8_t reg)
{
    *IO_OPL2REG = reg;
}

void OPL2WriteVal(uint8_t val)
{
	*IO_OPL2VAL = val;
}
