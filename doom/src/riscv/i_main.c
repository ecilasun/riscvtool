/*
 * i_main.c
 *
 * Main entry point
 *
 * Copyright (C) 2021 Sylvain Munaut
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// Modified to work on E32E by Engin Cilasun

#include "../doomdef.h"
#include "../d_main.h"
#include <stdlib.h>

const char *FRtoString[]={
	"Succeeded\n",
	"A hard error occurred in the low level disk I/O layer\n",
	"Assertion failed\n",
	"The physical drive cannot work\n",
	"Could not find the file\n",
	"Could not find the path\n",
	"The path name format is invalid\n",
	"Access denied due to prohibited access or directory full\n",
	"Access denied due to prohibited access\n",
	"The file/directory object is invalid\n",
	"The physical drive is write protected\n",
	"The logical drive number is invalid\n",
	"The volume has no work area\n",
	"There is no valid FAT volume\n",
	"The f_mkfs() aborted due to any problem\n",
	"Could not get a grant to access the volume within defined period\n",
	"The operation is rejected according to the file sharing policy\n",
	"LFN working buffer could not be allocated\n",
	"Number of open files > FF_FS_LOCK\n",
	"Given parameter is invalid\n"
};

#include "core.h"
#include "uart.h"
#include "ps2.h"
#include "fat32/ff.h"
#include "sdcard.h"

uint16_t keymap[256];
uint16_t keymapprev[256];

void HandleUART()
{
	// Echo back all data in the FIFO
	while (UARTHasData())
	{
		*IO_UARTTX = (uint32_t)UARTRead();
	}
}

void HandleKeyboard()
{
	// Consume all key state changes from FIFO and update the key map
	while (*PS2KEYBOARDDATAAVAIL)
		PS2ScanKeyboard(keymap);

	// If there's a difference between the previous keymap and current one, generate events for each change
	for (uint32_t i=0; i<256; ++i)
	{
		// Skip keyboard OK (when plugged in)
		if (i==0xAA)
			continue;

		// Generate key up/down events
		uint32_t prevval = (uint32_t)keymapprev[i];
		uint32_t val = (uint32_t)keymap[i];
		if (prevval^val) // Mismatch, this considered an event
		{
			// Store new state in previous state buffer since we're done reading it
			keymapprev[i] = val;
			// Wait for adequate space in ring buffer to write
			// NOTE: We'll simply hang if ringbuffer is full because nothing else
			// is running on this core during ISR. So attempt once, and bail out
			// if we can't write...
			//while(RingBufferWrite(&val, 4) == 0) { }
			// Ideally, this buffer should not overflow as long as the receiving app
			// is alive and well.
			PS2RingBufferWrite(&val, 4);
		}
	}
}

void __attribute__((aligned(16))) __attribute__((interrupt("machine"))) main_ISR()
{
	uint32_t value = read_csr(mtval);   // Instruction word or hardware bit
	uint32_t cause = read_csr(mcause);  // Exception cause on bits [18:16] (https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf)
	uint32_t PC = read_csr(mepc)-4;     // Return address minus one is the crash PC
	uint32_t code = cause & 0x7FFFFFFF;

	if (cause & 0x80000000) // Interrupt
	{
		if (code == 0xB) // Machine External Interrupt (hardware)
		{
			// Route based on hardware type
			if (value & 0x00000001) HandleUART();
			//if (value & 0x00000002) HandleButtons();
			if (value & 0x00000004) HandleKeyboard();
		}
	}
	else // Machine Software Exception (trap)
	{
		UARTWrite("\033[31m\033[40m");
		UARTWrite("┌───────────────────────────────────────────────────┐\n");
		UARTWrite("│ Software Failure. Press reset button to continue. │\n");
		UARTWrite("│   Guru Meditation #");
		UARTWriteHex((uint32_t)cause); // Cause
		UARTWrite(".");
		UARTWriteHex((uint32_t)value); // Failed instruction
		UARTWrite(" @");
		UARTWriteHex((uint32_t)PC); // PC
		UARTWrite("    │\n");
		UARTWrite("└───────────────────────────────────────────────────┘\n");
		UARTWrite("\033[0m\n");
		while(1) { asm volatile("wfi;"); } // Put core to endless sleep
	}
}

void InstallMainISR()
{
	// Set machine trap vector
	swap_csr(mtvec, main_ISR);

	// Enable machine software interrupts (breakpoint/illegal instruction)
	// Enable machine hardware interrupts
	// Disable machine timer interrupts
	swap_csr(mie, MIP_MSIP | MIP_MEIP);

	// Allow all machine interrupts to trigger
	swap_csr(mstatus, MSTATUS_MIE);
}

int main()
{
	// Disable all interrupts, reset some memory
	swap_csr(mie, 0);

	FATFS *Fs = (FATFS*)malloc(sizeof(FATFS));
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		printf(FRtoString[mountattempt]);
	else
	{
		printf("Setting up system\n");
		PS2InitRingBuffer();
		InstallMainISR();

		printf("Starting DOOM\n");
		D_DoomMain();
	}

	return 0;
}
