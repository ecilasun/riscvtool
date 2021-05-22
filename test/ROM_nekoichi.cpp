// Bootloader

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#define STARTUP_ROM
#include "rom_nekoichi_rvcrt0.h"

#include "utils.h"
#include "gpu.h"

#include "SDCARD.h"

volatile uint32_t gpuSideSubmitCounter = 0x00000000;
uint32_t gpuSubmitCounter = 0;
uint32_t vramPage = 0;

void DrawRotatingRect(const float ox, const float oy)
{
   static float roll = 1.8f;

   float rx0 = cosf(roll)*12.f;
   float ry0 = sinf(roll)*12.f;
   float rx1 = cosf(roll+3.1415927f*0.5f)*12.f;
   float ry1 = sinf(roll+3.1415927f*0.5f)*12.f;

   short x1 = short(ox - rx0 - rx1);
   short y1 = short(oy - ry0 - ry1);
   short x2 = short(ox + rx0 - rx1);
   short y2 = short(oy + ry0 - ry1);
   short x3 = short(ox + rx0 + rx1);
   short y3 = short(oy + ry0 + ry1);
   short x4 = short(ox - rx0 + rx1);
   short y4 = short(oy - ry0 + ry1);

   uint32_t vertex0 = ((y1&0xFFFF)<<16) | (x1&0xFFFF);
   uint32_t vertex1 = ((y2&0xFFFF)<<16) | (x2&0xFFFF);
   uint32_t vertex2 = ((y3&0xFFFF)<<16) | (x3&0xFFFF);
   uint32_t vertex3 = ((y4&0xFFFF)<<16) | (x4&0xFFFF);

   GPUSetRegister(1, vertex0);
   GPUSetRegister(2, vertex1);
   GPUSetRegister(3, vertex2);
   GPUSetRegister(4, vertex3);
   uint8_t ncolor = 0x37;
   GPURasterizeTriangle(1,2,3,ncolor);
   GPURasterizeTriangle(3,4,1,ncolor^0xFF);

   roll += 0.055f;
}

void SubmitGPUFrame()
{
   const uint8_t bgcolor = 0x03;//0x7C;
   const uint32_t colorbits = (bgcolor<<24) | (bgcolor<<16) | (bgcolor<<8) | bgcolor;

   if (gpuSideSubmitCounter == gpuSubmitCounter)
   {
      ++gpuSubmitCounter;

      // CLS
      GPUSetRegister(1, colorbits);
      GPUClearVRAMPage(1);

      DrawRotatingRect(128, 96);

      // Stall GPU until vsync is reached
      GPUWaitForVsync();

      // Swap to other page to reveal previous render
      vramPage = (vramPage+1)%2;
      GPUSetRegister(2, vramPage);
      GPUSetVideoPage(2);

      // GPU will write value in G2 to address in G3 in the future
      GPUSetRegister(3, uint32_t(&gpuSideSubmitCounter));
      GPUSetRegister(2, gpuSubmitCounter);
      GPUWriteSystemMemory(2, 3);

      // Clear state, GPU will overwrite this when it reaches GPUSYSMEMOUT
      gpuSideSubmitCounter = 0;
   }
}

void loadbinaryblob()
{
   // Header data
   uint32_t loadlen = 0;
   uint32_t loadtarget = 0;
   char *loadlenaschar = (char*)&loadlen;
   char *loadtargetaschar = (char*)&loadtarget;

   // Target address
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         loadlenaschar[writecursor++] = readdata;
      }
   }

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         target[writecursor++] = readdata;
      }
   }
}

void runbinaryblob()
{
   // Header data
   uint32_t branchaddress = 0;
   char *branchaddressaschar = (char*)&branchaddress;

   // Data length
   uint32_t writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = *IO_UARTRXByteCount;
      if (bytecount != 0)
      {
         unsigned char readdata = *IO_UARTRX;
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   EchoUART("Starting...\r\n");

   // Set up stack pointer and branch to loaded executable's entry point (noreturn)
   // TODO: Can we work out the stack pointer to match the loaded ELF's layout?
   asm (
      "lw ra, %0 \n"
      "fmv.w.x	f0, zero \n"
      "fmv.w.x	f1, zero \n"
      "fmv.w.x	f2, zero \n"
      "fmv.w.x	f3, zero \n"
      "fmv.w.x	f4, zero \n"
      "fmv.w.x	f5, zero \n"
      "fmv.w.x	f6, zero \n"
      "fmv.w.x	f7, zero \n"
      "fmv.w.x	f8, zero \n"
      "fmv.w.x	f9, zero \n"
      "fmv.w.x	f10, zero \n"
      "fmv.w.x	f11, zero \n"
      "fmv.w.x	f12, zero \n"
      "fmv.w.x	f13, zero \n"
      "fmv.w.x	f14, zero \n"
      "fmv.w.x	f15, zero \n"
      "fmv.w.x	f16, zero \n"
      "fmv.w.x	f17, zero \n"
      "fmv.w.x	f18, zero \n"
      "fmv.w.x	f19, zero \n"
      "fmv.w.x	f20, zero \n"
      "fmv.w.x	f21, zero \n"
      "fmv.w.x	f22, zero \n"
      "fmv.w.x	f23, zero \n"
      "fmv.w.x	f24, zero \n"
      "fmv.w.x	f25, zero \n"
      "fmv.w.x	f26, zero \n"
      "fmv.w.x	f27, zero \n"
      "fmv.w.x	f28, zero \n"
      "fmv.w.x	f29, zero \n"
      "fmv.w.x	f30, zero \n"
      "fmv.w.x	f31, zero \n"
      "li x12, 0x0003FFF0 \n" // we're not coming back to the loader, set SP to end of RAM
      "mv sp, x12 \n"
      "ret \n"
      : 
      : "m" (branchaddress)
      : 
   );

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

int main()
{
   EchoUART("              vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("                  vvvvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrr       vvvvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr    vvvvvvvvvvvvvvvvvvvvvvvv\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vvvvvvvvvvvvvvvvvvvvvv  \r\n");
   EchoUART("rrrrrrrrrrrrr       vvvvvvvvvvvvvvvvvvvvvv    \r\n");
   EchoUART("rr                vvvvvvvvvvvvvvvvvvvvvv      \r\n");
   EchoUART("rr            vvvvvvvvvvvvvvvvvvvvvvvv      rr\r\n");
   EchoUART("rrrr      vvvvvvvvvvvvvvvvvvvvvvvvvv      rrrr\r\n");
   EchoUART("rrrrrr      vvvvvvvvvvvvvvvvvvvvvv      rrrrrr\r\n");
   EchoUART("rrrrrrrr      vvvvvvvvvvvvvvvvvv      rrrrrrrr\r\n");
   EchoUART("rrrrrrrrrr      vvvvvvvvvvvvvv      rrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrr      vvvvvvvvvv      rrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrr      vvvvvv      rrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrr      vv      rrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrr          rrrrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrrrr      rrrrrrrrrrrrrrrrrrrr\r\n");
   EchoUART("rrrrrrrrrrrrrrrrrrrrrr  rrrrrrrrrrrrrrrrrrrrrr\r\n");

   EchoUART("\r\nNekoIchi [v002] [rv32imf] [GPU]\r\n");
   EchoUART("(c)2021 Engin Cilasun\r\n");

   // Initialize video page
   GPUSetRegister(2, vramPage);
   GPUSetVideoPage(2);

   // UART communication section
   uint8_t prevchar = 0xFF;
   while(1)
   {
      // Step 1: Read UART FIFO byte count
      uint32_t bytecount = *IO_UARTRXByteCount;

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = *IO_UARTRX;

         if (checkchar == 13) // Enter followed by B (binary blob) or R (run blob)
         {
            // Load the incoming binary
            if (prevchar=='B')
               loadbinaryblob();
            if (prevchar=='R')
               runbinaryblob();
         }
         prevchar = checkchar;
      }

      SubmitGPUFrame();
   }

   return 0;
}
