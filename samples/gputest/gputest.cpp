#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>
#include <cmath>
#include "nekoichi.h"
#include "gpu.h"
#include "console.h"

#define MAX_PARTICLES 1024

void drawrect(const float ox, const float oy, float rate)
{
   const uint8_t ncolor = 0x7C;
   float roll = rate; //1.8f;

   float rx0 = cosf(roll)*64.f;
   float ry0 = sinf(roll)*64.f;
   float rx1 = cosf(roll+3.1415927f*0.5f)*32.f;
   float ry1 = sinf(roll+3.1415927f*0.5f)*32.f;

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
   GPURasterizeTriangle(1,2,3,ncolor);
   GPURasterizeTriangle(3,4,1,ncolor^0xFF);

   //roll += 0.01f;
}

void resetparticles(short *particles)
{
   for (int i=0;i<MAX_PARTICLES;++i)
   {
      particles[4*i+0] = Random()%256; // X position
      particles[4*i+1] = -32; // Y position, <-30 if dead
      particles[4*i+2] = 3 + (Random()%7); // Speed
      particles[4*i+3] = 192 + (Random()%32); // Barrier
   }
}

void drawparticles(short *particles)
{
   static int spawnerindex = 0;

   // Move particles, kill those that go past the barrier
   for (int i=0;i<MAX_PARTICLES;++i)
   {
      particles[4*i+1] += particles[4*i+2];
      if (particles[4*i+1] > particles[4*i+3])
         particles[4*i+1] = (Random()%24)-24;
   }

   // Spawn one particle if the current position is 'dead'
   spawnerindex = (spawnerindex + 1)%MAX_PARTICLES;
   if (particles[4*spawnerindex+1] < -32) // Dead
   {
      particles[4*spawnerindex+0] = Random()%256;
      //particles[4*spawnerindex+1] = (Random()%24)-24;
      particles[4*spawnerindex+2] = 3 + (Random()%7);
      particles[4*spawnerindex+3] = 192 + (Random()%32); // Barrier
   }

   // Draw the live particles
   static float proll = 0.f;
   short rx0 = cosf(proll)*3.f;
   short ry0 = sinf(proll)*3.f;
   short rx1 = cosf(proll+3.1415927f*0.5f)*3.f;
   short ry1 = sinf(proll+3.1415927f*0.5f)*3.f;
   for (int i=0;i<MAX_PARTICLES;++i)
   {
      if (i%64==0)
      {
         const float S = 3.f;
         rx0 = cosf(proll)*S;
         ry0 = sinf(proll)*S;
         rx1 = cosf(proll+3.1415927f*0.5f)*S;
         ry1 = sinf(proll+3.1415927f*0.5f)*S;
         proll -= 0.02f;
      }
      if (particles[4*i+1] != -1)
      {
         short ox = particles[4*i+0];
         short oy = particles[4*i+1];
         short x1 = ox + (-rx0 - rx1);
         short y1 = oy + (-ry0 - ry1);
         short x2 = ox + ( rx0 - rx1);
         short y2 = oy + ( ry0 - ry1);
         short x3 = ox + ( rx0 + rx1);
         short y3 = oy + ( ry0 + ry1);
         //short x4 = ox + (-rx0 + rx1);
         //short y4 = oy + (-ry0 + ry1);

         uint32_t vertex0 = ((y1&0xFFFF)<<16) | (x1&0xFFFF);
         uint32_t vertex1 = ((y2&0xFFFF)<<16) | (x2&0xFFFF);
         uint32_t vertex2 = ((y3&0xFFFF)<<16) | (x3&0xFFFF);
         //uint32_t vertex3 = ((y4&0xFFFF)<<16) | (x4&0xFFFF);
         GPUSetRegister(1, vertex0);
         GPUSetRegister(2, vertex1);
         GPUSetRegister(3, vertex2);
         //GPUSetRegister(4, vertex3);
         GPURasterizeTriangle(1,2,3,i&0xFF);
         //GPURasterizeTriangle(3,4,1,i&0xFF);
      }
   }
   proll -= 0.02f;
}

int main(int argc, char ** argv)
{
   InitFont();
   ClearConsole();

   // Set register g1 with color data
   uint8_t bgcolor = 0x00;
   uint32_t colorbits = (bgcolor<<24) | (bgcolor<<16) | (bgcolor<<8) | bgcolor;
   GPUSetRegister(7, colorbits);

   float x=128.f, y=96.f;
   float dx=1.5f, dy=2.75f;
   //int f=0;
   int m=0;

   // Make sure this lands in some unused area of the GraphicsRAM
   volatile uint32_t *gpustate = (volatile uint32_t *)GraphicsRAMStart;
   *gpustate = 0x0;
   unsigned int cnt = 0x00000000;

   uint32_t page = 0;
   GPUSetRegister(6, page);
   GPUSetVideoPage(6);

   // Set palette
   /*for (uint32_t i=0;i<255;++i)
   {
      int j=255-i;
      uint32_t color = (j<<16) | (j<<8) | j;
      GPUSetRegister(1, color);
      GPUSetPaletteEntry(1, i);
   }*/
   GPUSetRegister(1, MAKERGBPALETTECOLOR(255,255,255));
   GPUSetPaletteEntry(1, 255);

   // HINT: we can 'place' particle buffer in DDR3 memory, instead of getting it allocated statically
   //short *triparticles = (short*)0x02000000;// short[4*MAX_PARTICLES];
   // Or, we can statically allocate it and waste space in the binary
   short triparticles[4*MAX_PARTICLES];
   // Or even better, we can dynamically allocate it and make it land wherever
   //short *triparticles = new short[4*MAX_PARTICLES];

   resetparticles(triparticles);

   uint32_t prevtime = 0;
   while(1)
   {
      uint8_t c1 = 0xFF;//cnt&0xFF;
      uint8_t c2 = 0xC0;//cnt^0xFF;

      if (*gpustate == cnt) // GPU work complete, push more
      {
         ++cnt;

         GPUSetRegister(1, MAKERGBPALETTECOLOR(Random()%255,Random()%255,Random()%255));
         GPUSetPaletteEntry(1, 255);

         if ((x>255.f) || (x<0.f))
            dx=-dx;
         if ((y>192.f) || (y<0.f))
            dy=-dy;
         x+=dx;
         y+=dy;

         GPUClearVRAMPage(7);

         drawparticles(triparticles);

         uint64_t clk = ReadClock();
         uint32_t milliseconds = ClockToMs(clk);
         uint32_t hours, minutes, seconds;
         ClockMsToHMS(milliseconds, hours,minutes,seconds);

         float rate = (float)(milliseconds%0xFFFF)/1000.f;
         drawrect(x, y, rate);

         uint32_t colortwo = (c1<<24) | (c1<<16) | (c1<<8) | c1;
         GPUSetRegister(2, colortwo);

         uint32_t colorthree = (c2<<24) | (c2<<16) | (c2<<8) | c2;
         GPUSetRegister(3, colorthree);

         PrintDMA(m%60,64,"GPU PIPELINE TEST", false);
         if (prevtime!=seconds)
         {
            prevtime = seconds;
            m+=4;
         }

         SetConsoleCursor(0, 0);
         ClearConsoleRow();
         EchoConsole("TIME: ");
         EchoConsole(hours);
         EchoConsole(":");
         EchoConsole(minutes);
         EchoConsole(":");
         EchoConsole(seconds);

         DrawConsole();

         // Stall GPU until vsync is reached
         GPUWaitForVsync();

         // Swap video page
         page = (page+1)%2;
         GPUSetRegister(1, page);
         GPUSetVideoPage(1);

         // GPU status address in G1
         uint32_t gpustateDWORDaligned = uint32_t(gpustate);
         GPUSetRegister(1, gpustateDWORDaligned);

         // Write 'end of processing' from GPU so that CPU can resume its work
         GPUSetRegister(2, cnt);
         GPUWriteToGraphicsMemory(2, 1);

         *gpustate = 0;
      }
   }

   return 0;
}
