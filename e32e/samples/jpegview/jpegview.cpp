#include "core.h"
#include "gpu.h"
#include "sdcard.h"
#include "fat32/ff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nanojpeg/nanojpeg.h"

FATFS Fs;
uint32_t hardwareswitchstates, oldhardwareswitchstates;
uint32_t vramPage = 0;
int selectedjpegfile = 0;
int numjpegfiles = 0;
char *jpegfiles[64];

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

		uint64_t begin = E32ReadTime();
		nj_result_t jres = njDecode(rawjpeg, fp.obj.objsize);
		uint64_t end = E32ReadTime();
		uint32_t decodems = ClockToMs(end-begin);
		printf("decoded in %d ms\n", int(decodems));

		if (jres == NJ_OK)
		{
			begin = E32ReadTime();

			int W = njGetWidth();
			int H = njGetHeight();
			printf("width:%d height:%d color:%d\n", W, H, njIsColor());
			int iW = W>320 ? 320 : W;
			int iH = H>208 ? 208 : H;
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

						image[x+y*320] = (uint8_t)((B<<6) | (G<<3) | R);
					}
				}
			}
			else
			{
				// Grayscale
				for (int j=0;j<iH;++j)
					for (int i=0;i<iW;++i)
						image[i+j*320] = img[i+j*W];
			}

			end = E32ReadTime();
			uint32_t ditherms = ClockToMs(end-begin);
			printf("dithered in %d ms\n", int(ditherms));
		}
		else
			printf("failed to decode image\n");
	}

	njDone();

	// Write to FB0, show FB0
	FrameBufferSelect(0, 0);

	for (uint32_t i=0;i<80*240;++i)
		GPUFBWORD[i] = ((uint32_t*)image)[i];
}

void Setup()
{
	// Make RGB palette
   int target = 0;
   for (int b=0;b<4;++b)
	   for (int g=0;g<8;++g)
   	for (int r=0;r<8;++r)
      GPUPAL_32[target++] = MAKERGBPALETTECOLOR(r*32, g*32, b*64);
}

int main( int argc, char **argv )
{
	Setup();

	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt != FR_OK)
		return -1;

	// Set aside space for the decompressed image
	image = (uint8_t*)malloc(320*208*3*sizeof(uint8_t));

	DecodeJPEG("sd:test.jpg");

	return 0;
}
