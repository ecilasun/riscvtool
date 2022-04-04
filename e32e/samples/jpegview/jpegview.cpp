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

uint32_t *image;

#define min(_x_,_y_) (_x_) < (_y_) ? (_x_) : (_y_)
#define max(_x_,_y_) (_x_) > (_y_) ? (_x_) : (_y_)

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
				// Copy
				for (int y=0;y<iH;++y)
				{
					for (int x=0;x<iW;++x)
					{
						image[(x+y*320)*3+0] = img[(x+y*W)*3+0];
						image[(x+y*320)*3+1] = img[(x+y*W)*3+1];
						image[(x+y*320)*3+2] = img[(x+y*W)*3+2];
					}
				}

				// Error diffusion dither
				// Can also be applied at hardware scan-out time
				for (int y=0;y<iH-1;++y)
				{
					for (int x=1;x<iW-1;++x)
					{
						int oldR = image[(x+y*320)*3+0];
						int oldG = image[(x+y*320)*3+1];
						int oldB = image[(x+y*320)*3+2];
						int R = (oldR/32)<<5;
						int G = (oldG/32)<<5;
						int B = (oldB/64)<<6;
						image[(x+y*320)*3+0] = R;
						image[(x+y*320)*3+1] = G;
						image[(x+y*320)*3+2] = B;

						int errR = (oldR-R);
						int errG = (oldG-G);
						int errB = (oldB-B);

						image[(x+1+y*320)*3+0] = min(255,max(0,image[(x+1+y*320)*3+0] + errR));
						image[(x+1+y*320)*3+1] = min(255,max(0,image[(x+1+y*320)*3+1] + errG));
						image[(x+1+y*320)*3+2] = min(255,max(0,image[(x+1+y*320)*3+2] + errB));
					}
				}

				// Convert to indexed color
				for (int y=0;y<iH;++y)
				{
					for (int x=0;x<iW;++x)
					{
						int R = image[(x+y*320)*3+0]>>5;
						int G = image[(x+y*320)*3+1]>>5;
						int B = image[(x+y*320)*3+2]>>6;
						image[x+y*320] = (B<<6) | (G<<3) | R;
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

	// Write to FB0
	*GPUCTL = 0;

	for (uint32_t i=0;i<80*240;++i)
		GPUFBWORD[i] = image[i];

	// Show FB0 (write to FB1)
	*GPUCTL = 1;
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
	image = (uint32_t*)malloc(320*208*3*sizeof(uint32_t));

	DecodeJPEG("sd:test.jpg");

	return 0;
}
