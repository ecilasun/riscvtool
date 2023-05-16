#include "core.h"
#include "gpu.h"
#include "sdcard.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "nanojpeg.h"

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

	FILE *fp = fopen(fname, "rb");
	if (fp)
	{
		// Grab file size
		fpos_t pos, endpos;
		fgetpos(fp, &pos);
		fseek(fp, 0, SEEK_END);
		fgetpos(fp, &endpos);
		fsetpos(fp, &pos);
		uint32_t fsize = (uint32_t)endpos;

		printf("Reading %ld bytes\n", fsize);
		uint8_t *rawjpeg = (uint8_t *)malloc(fsize);
		fread(rawjpeg, fsize, 1, fp);
		fclose(fp);

		printf("Decoding image\n");
		nj_result_t jres = njDecode(rawjpeg, fsize);

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
	else
		printf("Could not open file %s\n", fname);

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

	// Set aside space for the decompressed image
    // NOTE: Video scanout buffer has to be aligned at 64 byte boundary
	image = GPUAllocateBuffer(640*480);

	struct EVideoContext vx;
    vx.m_vmode = EVM_640_Wide;
    vx.m_cmode = ECM_8bit_Indexed;
	GPUSetVMode(&vx, EVS_Enable);
	GPUSetWriteAddress(&vx, (uint32_t)image);
	GPUSetScanoutAddress(&vx, (uint32_t)image);
	GPUClearScreen(&vx, 0x03030303);

    GPUPrintString(&vx, 0, 16, "loading test.jpg", 0x7FFFFFFF);
    CFLUSH_D_L1;

	DecodeJPEG("sd:test.jpg");
    CFLUSH_D_L1;

	return 0;
}
