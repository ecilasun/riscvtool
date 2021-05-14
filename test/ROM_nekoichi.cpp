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
      uint32_t bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadtargetaschar[writecursor++] = readdata;
      }
   }

   // Data length
   writecursor = 0;
   while(writecursor < 4)
   {
      uint32_t bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         loadlenaschar[writecursor++] = readdata;
      }
   }

   // Read binary blob
   writecursor = 0;
   volatile unsigned char* target = (volatile unsigned char* )loadtarget;
   while(writecursor < loadlen)
   {
      uint32_t bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
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
      uint32_t bytecount = UARTRXStatus[0];
      if (bytecount != 0)
      {
         unsigned char readdata = UARTRX[0];
         branchaddressaschar[writecursor++] = readdata;
      }
   }

   // Old way: this doesn't set up anything before launching target executable
   // ((void (*)(void)) branchaddress)();

   // Set up stack pointer and branch to loaded executable's entry point (noreturn)
   asm (
      "lw ra, %0; \n"
      : 
      : "m" (branchaddress)
      : 
   );
   asm (
      "fmv.w.x	f0, zero; \n"
      "fmv.w.x	f1, zero; \n"
      "fmv.w.x	f2, zero; \n"
      "fmv.w.x	f3, zero; \n"
      "fmv.w.x	f4, zero; \n"
      "fmv.w.x	f5, zero; \n"
      "fmv.w.x	f6, zero; \n"
      "fmv.w.x	f7, zero; \n"
      "fmv.w.x	f8, zero; \n"
      "fmv.w.x	f9, zero; \n"
      "fmv.w.x	f10, zero; \n"
      "fmv.w.x	f11, zero; \n"
      "fmv.w.x	f12, zero; \n"
      "fmv.w.x	f13, zero; \n"
      "fmv.w.x	f14, zero; \n"
      "fmv.w.x	f15, zero; \n"
      "fmv.w.x	f16, zero; \n"
      "fmv.w.x	f17, zero; \n"
      "fmv.w.x	f18, zero; \n"
      "fmv.w.x	f19, zero; \n"
      "fmv.w.x	f20, zero; \n"
      "fmv.w.x	f21, zero; \n"
      "fmv.w.x	f22, zero; \n"
      "fmv.w.x	f23, zero; \n"
      "fmv.w.x	f24, zero; \n"
      "fmv.w.x	f25, zero; \n"
      "fmv.w.x	f26, zero; \n"
      "fmv.w.x	f27, zero; \n"
      "fmv.w.x	f28, zero; \n"
      "fmv.w.x	f29, zero; \n"
      "fmv.w.x	f30, zero; \n"
      "fmv.w.x	f31, zero; \n"
      "li x12, 0x0001FFF0; \n"
      "mv sp, x12; \n"
      "ret; \n"
   );

   // Unfortunately, if I use 'noreturn' attribute with above code, it doesn't work
   // and there'll be a redundant stack op and a ret generated here
}

void echoterm(const char *_message)
{
   int i=0;
   while (_message[i]!=0)
   {
      UARTTX[i] = _message[i];
      ++i;
   }
}

void drawrect(const float ox, const float oy)
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

   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(vertex0));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(vertex0));
   GPUFIFO[2] = GPUOPCODE(GPUSETREGISTER, 0, 2, GPU22BITIMM(vertex1));
   GPUFIFO[3] = GPUOPCODE(GPUSETREGISTER, 2, 2, GPU10BITIMM(vertex1));
   GPUFIFO[4] = GPUOPCODE(GPUSETREGISTER, 0, 3, GPU22BITIMM(vertex2));
   GPUFIFO[5] = GPUOPCODE(GPUSETREGISTER, 3, 3, GPU10BITIMM(vertex2));
   GPUFIFO[2] = GPUOPCODE(GPUSETREGISTER, 0, 4, GPU22BITIMM(vertex3));
   GPUFIFO[3] = GPUOPCODE(GPUSETREGISTER, 4, 4, GPU10BITIMM(vertex3));
   uint8_t ncolor = 0x37;
   GPUFIFO[6] = GPUOPCODE3(GPURASTERIZE, 1, 2, 3, (ncolor));
   GPUFIFO[6] = GPUOPCODE3(GPURASTERIZE, 3, 4, 1, (ncolor^0xFF));

   roll += 0.04f;
}

int main()
{
   echoterm("NekoIchi\r\nrv32imf@60Mhz\r\nv0001\r\n");

   volatile uint32_t gpustate = 0x00000000;
   uint32_t counter = 0;

   uint8_t bgcolor = 0x03;//0x7C;
   uint32_t colorbits = (bgcolor<<24) | (bgcolor<<16) | (bgcolor<<8) | bgcolor;

   // Set page
   uint32_t page = 0;
   GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 2, GPU22BITIMM(page));
   GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 2, 2, GPU10BITIMM(page));
   GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 2, 0, 0);

   uint32_t enableGPUworker = 1;

   // UART communication section
   uint8_t prevchar = 0xFF;
   while(1)
   {
      // Step 1: Read UART FIFO byte count
      uint32_t bytecount = UARTRXStatus[0];

      // Step 2: Check to see if we have something in the FIFO
      if (bytecount != 0)
      {
         // Step 3: Read the data on UARTRX memory location
         char checkchar = UARTRX[0];

         if (checkchar == 13) // Enter followed by B (binary blob) or R (run blob)
         {
            // Load the incoming binary
            if (prevchar=='B')
               loadbinaryblob();
            if (prevchar=='R')
               runbinaryblob();
         }
         prevchar = checkchar;

         // Echo the character back to the sender
         UARTTX[0] = checkchar;
         if (checkchar==13)
            UARTTX[1] = 10;

         // Stop rendering
         enableGPUworker = 0;
      }

      // Do this when idle
      if (enableGPUworker && (gpustate == counter))
      {
         ++counter;

         // CLS
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 1, GPU22BITIMM(colorbits));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 1, 1, GPU10BITIMM(colorbits));
         GPUFIFO[2] = GPUOPCODE(GPUCLEAR, 1, 0, 0);

         drawrect(128, 96);

         // Render text
         PrintDMA(56, 8, "Please upload ELF");
         PrintDMA(4, 16, "USB/UART @115200bps:8bit:1st:np");

         // Stall GPU until vsync is reached
         GPUFIFO[4] = GPUOPCODE(GPUVSYNC, 0, 0, 0);

         // Swap to other page to reveal previous render
         page = (page+1)%2;
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 2, GPU22BITIMM(page));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 2, 2, GPU10BITIMM(page));
         GPUFIFO[2] = GPUOPCODE(GPUSETVPAGE, 2, 0, 0);

         // GPU status address in G3
         uint32_t gpustateDWORDaligned = uint32_t(&gpustate);
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 3, GPU22BITIMM(gpustateDWORDaligned));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 3, 3, GPU10BITIMM(gpustateDWORDaligned));

         // Will write incremented counter in the future from GPU side
         GPUFIFO[0] = GPUOPCODE(GPUSETREGISTER, 0, 2, GPU22BITIMM(counter));
         GPUFIFO[1] = GPUOPCODE(GPUSETREGISTER, 2, 2, GPU10BITIMM(counter));
         GPUFIFO[4] = GPUOPCODE(GPUSYSMEMOUT, 2, 3, 0);

         // Clear state, GPU will overwrite this when it reaches GPUSYSMEMOUT
         gpustate = 0;
      }

   }

   return 0;
}
