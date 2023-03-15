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

#include "basesystem.h"
#include "uart.h"
#include "fat32/ff.h"
#include "sdcard.h"

int main()
{
	FATFS *Fs = (FATFS*)malloc(sizeof(FATFS));
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		printf(FRtoString[mountattempt]);
	else
		D_DoomMain();

	return 0;
}
