// MiniTerm

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "core.h"
#include "gpu.h"
#include "uart.h"
#include "sdcard.h"
#include "console.h"
#include "elf.h"
#include "ringbuffer.h"
#include "ps2.h"

uint8_t *keyboardringbuffer = (uint8_t*)0x80000200; // 512 bytes into mailbox memory

int main()
{
	const uint32_t bgcolor = 0x1A1A1A1A; // Use default VGA palette grayscale value

	char cmdbuffer[(CONSOLE_COLUMNS+1)*(CONSOLE_ROWS+1)];

	// Set output page
	uint32_t page = 0;
	FrameBufferSelect(page, page^1);

	// Startup message
	ClearConsole();
	ClearScreen(bgcolor);
	EchoConsole("MiniTerm (c)2021 Engin Cilasun\n Use 'help' for assistance\n");
	DrawConsole();

	uint32_t prevmilliseconds = 0;
	uint32_t cursorevenodd = 0;
	uint32_t toggletime = 0;
	uint32_t uppercase = 0;

	while(1)
	{
		uint64_t clk = E32ReadTime();
		uint32_t milliseconds = ClockToMs(clk);
		uint32_t hours, minutes, seconds;
		ClockMsToHMS(milliseconds, &hours,&minutes,&seconds);

		// Check keyboard entry
		uint32_t val;
		swap_csr(mie, MIP_MSIP | MIP_MTIP);
		int R = RingBufferRead(keyboardringbuffer, (uint8_t*)&val, 4);
		swap_csr(mie, MIP_MSIP | MIP_MEIP | MIP_MTIP);

		if (R)
		{
			uint32_t key = val&0xFF;

			// Toggle caps
			if (key == 0x58)
				uppercase = !uppercase;

			// Key up
			if (val&0x00000100)
			{
				if (key == 0x12 || key == 0x59) // Right/left shift up
					uppercase = 0;
			}
			else // Key down
			{
				if (key == 0x12 || key == 0x59) // Right/left shift down
					uppercase = 1;
				else
				{
					int cx, cy;
					GetConsoleCursor(&cx, &cy);
					if (key == 0x5A) // Enter
					{
						// First, copy current row
						ConsoleStringAtRow(cmdbuffer);
						// Next, send a newline to go down one
						EchoConsole("\n");

						// Clear the whole screen
						if ((cmdbuffer[0]='c') && (cmdbuffer[1]=='l') && (cmdbuffer[2]=='s'))
						{
							ClearConsole();
						}
						else if ((cmdbuffer[0]='h') && (cmdbuffer[1]=='e') && (cmdbuffer[2]=='l') && (cmdbuffer[3]=='p'))
						{
							EchoConsole("\n");
							EchoConsole("\nMiniTerm version 0.1\n(c)2021 Engin Cilasun\ncls: clear screen\nhelp: help screen\ntime: elapsed time\ndump:dump first 256 bytes of ELF\n");
						}
						else if ((cmdbuffer[0]='d') && (cmdbuffer[1]=='u') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='p'))
						{
							for (uint32_t i=0x0A000000; i<0x0A000100; i+=4)
							{
								EchoConsoleHex(i);
								EchoConsole(":");
								EchoConsoleHex(*((uint32_t*)i));
								EchoConsole("\n");
							}
						}
						else if ((cmdbuffer[0]='t') && (cmdbuffer[1]=='i') && (cmdbuffer[2]=='m') && (cmdbuffer[3]=='e'))
							toggletime = (toggletime+1)%2;
					}
					else if (key == 0x66) // Backspace
					{
						ConsoleCursorStepBack();
						EchoConsole(" ");
						ConsoleCursorStepBack();
					}
					else if (key == 0x74) // right
						SetConsoleCursor(cx+1, cy);
					else if (key == 0x6B) // left
						SetConsoleCursor(cx-1, cy);
					else if (key == 0x75) // up
						SetConsoleCursor(cx, cy-1);
					else if (key == 0x72) // down
						SetConsoleCursor(cx, cy+1);
					else
					{
						char shortstring[2];
						shortstring[0]=ScanToASCII(val, uppercase);
						shortstring[1]=0;
						EchoConsole(shortstring);
					}
				}
			}
		}

		// Show the char table
		ClearScreen(bgcolor);
		DrawConsole();

		// Cursor blink
		if (milliseconds-prevmilliseconds > 530)
		{
			prevmilliseconds = milliseconds;
			++cursorevenodd;
		}

		// Cursor overlay
		if ((cursorevenodd%2) == 0)
		{
			int cx, cy;
			GetConsoleCursor(&cx, &cy);
			DrawText(cx*2, cy*8, "_");
		}

		if (toggletime)
		{
			uint32_t offst = DrawDecimal(0, 0, hours);
			DrawText(offst*2, 0, ":"); ++offst;
			offst += DrawDecimal(offst*2,0,minutes);
			DrawText(offst*2, 0, ":"); ++offst;
			offst += DrawDecimal(offst*2,0,seconds);
		}

		page = (page+1)%2;
		FrameBufferSelect(page, page^1);
   }

   return 0;
}
