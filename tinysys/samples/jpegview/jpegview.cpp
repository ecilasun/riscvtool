#include "core.h"
#include "gpu.h"
#include "sdcard.h"
#include "fat32/ff.h"
#include "uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nanojpeg/nanojpeg.h"

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

uint8_t *image;

#define min(_x_,_y_) (_x_) < (_y_) ? (_x_) : (_y_)
#define max(_x_,_y_) (_x_) > (_y_) ? (_x_) : (_y_)

// The Bayer matrix for ordered dithering
const uint8_t dither[4][4] = {
  { 0, 8, 2,10},
  {12, 4,14, 6},
  { 3,11, 1, 9},
  {15, 7,13, 5}
};

void DecodeJPEG(const char *fname)
{
	njInit();

	FIL fp;
	FRESULT fr = f_open(&fp, fname, FA_READ);
	if (fr == FR_OK)
	{
		UINT readlen;
		uint8_t *rawjpeg = (uint8_t *)malloc(fp.obj.objsize);
		f_read(&fp, rawjpeg, fp.obj.objsize, &readlen);
		f_close(&fp);

		nj_result_t jres = njDecode(rawjpeg, fp.obj.objsize);

		if (jres == NJ_OK)
		{
			int W = njGetWidth();
			int H = njGetHeight();

			int iW = W>640 ? 640 : W;
			int iH = H>479 ? 479 : H;
			uint8_t *img = njGetImage();
			if (njIsColor())
			{
				// Copy, dither and convert to indexed color
				for (int y=0;y<iH;++y)
				{
					for (int x=0;x<iW;++x)
					{
						uint8_t R = img[(x+y*W)*3+0];
						uint8_t G = img[(x+y*W)*3+1];
						uint8_t B = img[(x+y*W)*3+2];

						uint8_t ROFF = min(dither[x&3][y&3] + R, 255);
						uint8_t GOFF = min(dither[x&3][y&3] + G, 255);
						uint8_t BOFF = min(dither[x&3][y&3] + B, 255);

						R = ROFF/32;
						G = GOFF/32;
						B = BOFF/64;

						image[x+y*640] = (uint8_t)((B<<6) | (G<<3) | R);
					}
				}
			}
			else
			{
				// Grayscale
				for (int j=0;j<iH;++j)
					for (int i=0;i<iW;++i)
						image[i+j*640] = img[i+j*W];
			}
		}
	}

	njDone();
}

void Setup()
{
	// Make RGB palette
	int target = 0;
	for (int b=0;b<4;++b)
		for (int g=0;g<8;++g)
			for (int r=0;r<8;++r)
				GPUSetPal(target++, r*36, g*36, b*85);
}

int main()
{
	Setup();

	FATFS *Fs = (FATFS*)malloc(sizeof(FATFS));
	FRESULT mountattempt = f_mount(Fs, "sd:", 1);
	if (mountattempt != FR_OK)
	{
		UARTWrite(FRtoString[mountattempt]);
		return -1;
	}

	// Set aside space for the decompressed image
    // NOTE: Video scanout buffer has to be aligned at 64 byte boundary
	image = GPUAllocateBuffer(640*480);

	struct EVideoContext vx;
	GPUSetVMode(&vx, EVM_640_Pal, EVS_Enable);
	GPUSetWriteAddress(&vx, (uint32_t)image);
	GPUSetScanoutAddress(&vx, (uint32_t)image);
	GPUClearScreen(&vx, 0x03030303);

    GPUPrintString(&vx, 0, 16, "loading test.jpg", 0x7FFFFFFF);
    CFLUSH_D_L1;

	DecodeJPEG("sd:test.jpg");
    CFLUSH_D_L1;

	return 0;
}