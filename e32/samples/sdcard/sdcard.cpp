#include "core.h"
#include "uart.h"

#include "spi.h"
#include "sdcard.h"
#include "fat32/ff.h"

#include <string.h>
#include <stdio.h>

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

// Shared FAT file system at bottom of S-RAM
FATFS *Fs = (FATFS*)0x8002F000;

void ListELF(const char *path)
{
   DIR dir;
   FRESULT re = f_opendir(&dir, path);
   if (re == FR_OK)
   {
      FILINFO finf;
      do{
         re = f_readdir(&dir, &finf);
         if (re == FR_OK && dir.sect!=0)
         {
			if (strstr(finf.fname, ".elf"))
			{
            	UARTWrite(finf.fname);
				UARTWrite(" ");
				UARTWriteDecimal((int32_t)finf.fsize);
				UARTWrite("b\n");
			}
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      UARTWrite(FRtoString[re]);
}

int main()
{
    UARTWrite("SD Card test\n\n");

	// Init file system
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		UARTWrite(FRtoString[mountattempt]);
	else
	{
		UARTWrite("Listing ELF files on sd:\n");
		// List ELF files on the mounted volume
		ListELF("sd:");
	}

    return 0;
}
