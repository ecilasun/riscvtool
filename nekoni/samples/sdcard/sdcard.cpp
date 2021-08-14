#include "core.h"
#include "uart.h"

#include "spi.h"
#include "sdcard.h"
#include "fat32/ff.h"

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

int main()
{
    EchoUART("SD Card startup...\n");

    if (SDCardStartup() == -1)
        EchoUART("failed\n");
    else
        EchoUART("done\n");

    // Init file system
    /*FATFS Fs;
    FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
    if (mountattempt!=FR_OK)
        EchoUART(FRtoString[mountattempt]);
    else
        EchoUART("Mounted volume SD:\n");*/

    // Stay here, as we don't have anywhere else to go back to
    while (1) { }

    return 0;
}
