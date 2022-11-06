#include "xadc.h"

volatile uint32_t *XADCAUX0 = (volatile uint32_t* )0x80000020;
volatile uint32_t *XADCTEMP = (volatile uint32_t* )0x80000024;