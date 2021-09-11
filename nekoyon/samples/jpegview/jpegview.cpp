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

GPUCommandPackage gpuSetupProg;

uint32_t *image;

#define min(_x_,_y_) (_x_) < (_y_) ? (_x_) : (_y_)
#define max(_x_,_y_) (_x_) > (_y_) ? (_x_) : (_y_)

void SubmitDecodedFrame()
{
	/*{
		// CLS
		GPUSetRegister(1, 0x10101010);
		GPUClearVRAMPage(1);

      // DMA
      for (uint32_t j=0;j<208;++j)
      {
         uint32_t sysramsource = uint32_t(GRAMStart)+j*320;
         GPUSetRegister(4, sysramsource);
         uint32_t vramramtarget = (j*512)>>2;
         GPUSetRegister(5, vramramtarget);
         uint32_t dmacount = 80;
         GPUKickDMA(4, 5, dmacount, false);
      }

      //GPUWaitForVsync();

		// Swap to other page to reveal previous render
		vramPage = (vramPage+1)%2;
		GPUSetRegister(2, vramPage);
		GPUSetVideoPage(2);
	}*/
}

void SubmitGPUFrame()
{
   // Do not submit more work if the GPU is not ready
   /*{
      // CLS
      GPUSetRegister(1, 0x16161616); // 4 dark gray pixels
      GPUClearVRAMPage(1);

      if (numjpegfiles == 0)
      {
         PrintDMA(88, 92, "jpeg decoder", false);
      }
      else
      {
         int line = 0;
         for (int i=0;i<numjpegfiles;++i)
         {
            PrintDMA(8, line, jpegfiles[i], (selectedjpegfile == i) ? false : true);
            line += 8;
         }
      }

      // Stall GPU until vsync is reached
      //GPUWaitForVsync();

      // Swap to other page to reveal previous render
      vramPage = (vramPage+1)%2;
      GPUSetRegister(2, vramPage);
      GPUSetVideoPage(2);
   }*/
}

void ListDir(const char *path)
{
   numjpegfiles = 0;
   DIR dir;
   FRESULT re = f_opendir(&dir, path);
   if (re == FR_OK)
   {
      FILINFO finf;
      do{
         re = f_readdir(&dir, &finf);
         if (re == FR_OK && dir.sect!=0)
         {
            // We're only interested in JPEG files
            if (strstr(finf.fname,".jpg"))
            {
               int olen = strlen(finf.fname) + 3;
               jpegfiles[numjpegfiles] = (char*)malloc(olen+1);
               strcpy(jpegfiles[numjpegfiles], "sd:");
               strcat(jpegfiles[numjpegfiles], finf.fname);
               jpegfiles[numjpegfiles][olen] = 0;
               ++numjpegfiles;
            }
         }
      } while(re == FR_OK && dir.sect!=0);
      f_closedir(&dir);
   }
}

void DecodeJPEG(int file)
{
   njInit();

   FIL fp;
   FRESULT fr = f_open(&fp, jpegfiles[file], FA_READ);
   if (fr == FR_OK)
   {
      UINT readlen;
      uint8_t *rawjpeg = (uint8_t *)malloc(fp.obj.objsize);
      f_read(&fp, rawjpeg, fp.obj.objsize, &readlen);
      f_close(&fp);

      uint64_t begin = ReadClock();
      nj_result_t jres = njDecode(rawjpeg, fp.obj.objsize);
      uint64_t end = ReadClock();
      uint32_t decodems = ClockToMs(end-begin);
      printf("decoded in %d ms\n", int(decodems));

      if (jres == NJ_OK)
      {
         begin = ReadClock();

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
                  GRAMStart[x+y*320] = (B<<6) | (G<<3) | R;
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

         end = ReadClock();
         uint32_t ditherms = ClockToMs(end-begin);
         printf("dithered in %d ms\n", int(ditherms));
      }
      else
         printf("failed to decode image\n");
   }

   /*int done = 0;
   while(!done)
   {
      // Down arrow to go back
      hardwareswitchstates = *IO_SwitchState;
      done = 0;
      switch(hardwareswitchstates ^ oldhardwareswitchstates)
      {
         case 0x20: done = hardwareswitchstates&0x20 ? 0 : 1; break;
         default: break;
      };

      SubmitDecodedFrame();
   }*/

   njDone();
}

void Setup()
{
	// Make RGB palette
   int target = 0;
   for (int b=0;b<4;++b)
   for (int g=0;g<8;++g)
   for (int r=0;r<8;++r)
      GRAMStart[target++] = MAKERGBPALETTECOLOR(r*32, g*32, b*64);

    GPUInitializeCommandPackage(&gpuSetupProg);
    GPUWritePrologue(&gpuSetupProg);
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_MISC, G_R0, 0x0, 0x0, G_VPAGE)); // Write to page 0
    GPUWriteInstruction(&gpuSetupProg, GPU_INSTRUCTION(G_DMA, G_R0, G_R0, G_DMAGRAMTOPALETTE, 0x100)); // move palette from top of G-RAM to palette memory at 0
    GPUWriteEpilogue(&gpuSetupProg);
    GPUCloseCommandPackage(&gpuSetupProg);

    GPUClearMailbox();
    GPUSubmitCommands(&gpuSetupProg);
    GPUKick();
    GPUWaitMailbox();
}

int main( int argc, char **argv )
{
   Setup();

	// Read switch state at startup
	//hardwareswitchstates = oldhardwareswitchstates = *IO_SwitchState;

	//InitFont();

	// Initialize video page
	//GPUSetRegister(2, vramPage);
	//GPUSetVideoPage(2);

	FRESULT mountattempt = f_mount(&Fs, "sd:", 1);
	if (mountattempt != FR_OK)
		return -1;

   ListDir("sd:");

   // Set aside space for the decompressed image
   image = (uint32_t*)malloc(320*208*3*sizeof(uint32_t));

	/*while(1)
	{
		hardwareswitchstates = *IO_SwitchState;

		int down = 0, up = 0, sel = 0;
		switch(hardwareswitchstates ^ oldhardwareswitchstates)
		{
			case 0x10: sel = hardwareswitchstates&0x10 ? 0 : 1; break;
			case 0x20: down = hardwareswitchstates&0x20 ? 0 : 1; break;
			case 0x40: up = hardwareswitchstates&0x40 ? 0 : 1; break;
			default: break;
		};

		oldhardwareswitchstates = hardwareswitchstates;

		if (up) selectedjpegfile--;
		if (down) selectedjpegfile++;
		if (selectedjpegfile>=numjpegfiles) selectedjpegfile = 0;
		if (selectedjpegfile<0) selectedjpegfile = numjpegfiles-1;
		if (sel) { DecodeJPEG(selectedjpegfile); }

		SubmitGPUFrame();
	}*/

	return 0;
}
