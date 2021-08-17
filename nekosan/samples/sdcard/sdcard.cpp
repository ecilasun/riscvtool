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

void ListDir(const char *path)
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
            /*int fidx=0;
            char flags[64]="";
            if (finf.fattrib&0x01) flags[fidx++]='r';
            if (finf.fattrib&0x02) flags[fidx++]='h';
            if (finf.fattrib&0x04) flags[fidx++]='s';
            if (finf.fattrib&0x08) flags[fidx++]='l';
            if (finf.fattrib&0x0F) flags[fidx++]='L';
            if (finf.fattrib&0x10) flags[fidx++]='d';
            if (finf.fattrib&0x20) flags[fidx++]='a';
            flags[fidx++]=0;*/
            EchoStr(finf.fname);
			EchoStr(" ");
			EchoDec((int32_t)finf.fsize);//, flags);
			EchoStr("b\n");
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
   else
      EchoStr(FRtoString[re]);
}

int main()
{
    EchoStr("SD Card test\n\n");

	// Init file system
	FATFS Fs;
	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt!=FR_OK)
		EchoStr(FRtoString[mountattempt]);
	else
	{
		EchoStr("Mounted volume SD:\n");
		ListDir("sd:");
	}

    return 0;
}
