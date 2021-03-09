// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include "utils.h"
#include "SDCARD.h"
#include "FAT.h"

// OS calls:
// _exit, close, environ, execve, fork, fstat, getpid, isatty, kill, link, lseek, open, read, sbrk, stat, times, unlink, wait, write

// Accessing the linker sections:
// uint8_t *data_byte = &_sdata;

const char hexdigits[] = "0123456789ABCDEF";
int main(int argc, char ** argv)
{
   ClearScreen(0xC8);

   SDCardStartup();

   fat_getpartition();

   /*FILEHANDLE fh;
   //if (fat_find_file("MINITERMELF", &fh))
   if (fat_openfile("MINITERMELF", &fh))
      Print(0, 0, "File found");
   else
      Print(0, 0, "File not found");*/

   fat_list_files(nullptr);

   /*int cnt=0;
   int x=0, y=0;
   int dx=3, dy=2;
   int f=0;*/
   while(1)
   {
   }
   return 0;
}
