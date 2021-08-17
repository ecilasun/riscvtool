#include "core.h"
#include "spi.h"
#include "sdcard.h"
#include <stdio.h>

void SDStrobe8()
{
   /*
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = 0xFF;*/
}

static uint8_t CRC7(const uint8_t* data, uint8_t n) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < n; i++) {
    uint8_t d = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc <<= 1;
      if ((d & 0x80) ^ (crc & 0x80)) {
        crc ^= 0x09;
      }
      d <<= 1;
    }
  }
  return (crc << 1) | 1;
}

void SDCmd(const SDCardCommand cmd, uint32_t args)
{
   uint8_t buf[8];

   buf[0] = SPI_CMD(cmd);
   buf[1] = (uint8_t)((args&0xFF000000)>>24);
   buf[2] = (uint8_t)((args&0x00FF0000)>>16);
   buf[3] = (uint8_t)((args&0x0000FF00)>>8);
   buf[4] = (uint8_t)(args&0x000000FF);
   buf[5] = CRC7(buf, 5);

   *IO_SPIRXTX = 0xFF;
   for (uint32_t i=0;i<6;++i)
      *IO_SPIRXTX = buf[i];
}

uint8_t SDIdle()
{
   register uint32_t oldmsie;
   register uint32_t msiedisable = 0;
	asm volatile("csrrw %0, mie, %1" : "=r"(oldmsie) : "r" (msiedisable));

   uint8_t response;

   // Enter idle state
   SDCmd(CMD0_GO_IDLE_STATE, 0);

   int timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01

   SDStrobe8();

	asm volatile("csrrw zero, mie,%0" :: "r" (oldmsie));

   return response;
}

uint8_t SDCheckVoltageRange(uint32_t *databack)
{
   register uint32_t oldmsie;
   register uint32_t msiedisable = 0;
	asm volatile("csrrw %0, mie, %1" : "=r"(oldmsie) : "r" (msiedisable));

   uint8_t response;

   SDCmd(CMD8_SEND_IF_COND, 0x000001AA);

   int timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card) - got 0x01

   // Read the 00 00 01 AA sequence back from the SD CARD
   *databack = 0x00000000;
   *IO_SPIRXTX = 0xFF;
   *databack |= *IO_SPIRXTX;
   *IO_SPIRXTX = 0xFF;
   *databack |= (*databack<<8)|(*IO_SPIRXTX);
   *IO_SPIRXTX = 0xFF;
   *databack |= (*databack<<8)|(*IO_SPIRXTX);
   *IO_SPIRXTX = 0xFF;
   *databack |= (*databack<<8)|(*IO_SPIRXTX);

   SDStrobe8();

	asm volatile("csrrw zero, mie,%0" :: "r" (oldmsie));

   return response;
}

uint8_t SDCardInit()
{
   register uint32_t oldmsie;
   register uint32_t msiedisable = 0;
	asm volatile("csrrw %0, mie, %1" : "=r"(oldmsie) : "r" (msiedisable));

   uint8_t response;

   // ACMD header
   SDCmd(CMD55_APP_CMD, 0x00000000);

   int timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00?? - NOTE: Won't handle old cards!

   // Set high capacity mode on
   *IO_SPIRXTX = 0xFF;
   SDCmd(ACMD41_SD_SEND_OP_COND, 0x40000000);

   timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00 eventually, but will also get several 0x01 (idle)

   SDStrobe8();

   // Initialize
   /**IO_SPIRXTX = 0xFF;
   *IO_SPIRXTX = SPI_CMD(CMD1_SEND_OP_COND);
   *IO_SPIRXTX = 0x00;
   *IO_SPIRXTX = 0x00;
   *IO_SPIRXTX = 0x00;
   *IO_SPIRXTX = 0x00;
   *IO_SPIRXTX = 0xFF; // checksum is not necessary at this point

   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
   } while(1); // Expected: 0x00*/

	asm volatile("csrrw zero, mie,%0" :: "r" (oldmsie));

   return response;
}

uint8_t SDSetBlockSize512()
{
   register uint32_t oldmsie;
   register uint32_t msiedisable = 0;
	asm volatile("csrrw %0, mie, %1" : "=r"(oldmsie) : "r" (msiedisable));

   uint8_t response;

   // Set block length
   SDCmd(CMD16_SET_BLOCKLEN, 0x00000200);

   int timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   SDStrobe8();

	asm volatile("csrrw zero, mie,%0" :: "r" (oldmsie));

   return response;
}

uint8_t SDReadSingleBlock(uint32_t sector, uint8_t *datablock, uint8_t checksum[2])
{
   register uint32_t oldmsie;
   register uint32_t msiedisable = 0;
	asm volatile("csrrw %0, mie, %1" : "=r"(oldmsie) : "r" (msiedisable));

   uint8_t response;

   // Read single block
   // NOTE: sector<<9 for non SDHC cards
   SDCmd(CMD17_READ_SINGLE_BLOCK, sector);

   // R1: expect 0x00
   int timeout=65536;
   do {
      *IO_SPIRXTX = 0xFF;
      response = *IO_SPIRXTX;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0);

   if (response != 0xFF) // == 0x00
   {
      // R2: expect 0xFE
      timeout=65536;
      do {
         *IO_SPIRXTX = 0xFF;
         response = *IO_SPIRXTX;
         if (response != 0xFF)
            break;
         --timeout;
      } while(timeout>0);

      if (response == 0xFE)
      {
         // Data burst follows
         // 512 bytes of data followed by 16 bit CRC, total of 514 bytes
         int x=0;
         do {
            *IO_SPIRXTX = 0xFF;
            datablock[x++] = *IO_SPIRXTX;
         } while(x<512);

         // Checksum
         *IO_SPIRXTX = 0xFF;
         checksum[0] = *IO_SPIRXTX;
         *IO_SPIRXTX = 0xFF;
         checksum[1] = *IO_SPIRXTX;
      }
   }

   SDStrobe8();

	asm volatile("csrrw zero, mie,%0" :: "r" (oldmsie));

   // Error
   /*if (!(response&0xF0))
   {
      if (response&0x01)
         I_Error("SDReadSingleBlock: error response = 'error'");
      if (response&0x02)
         I_Error("SDReadSingleBlock: error response = 'CC error'");
      if (response&0x04)
         I_Error("SDReadSingleBlock: error response = 'Card ECC failed'");
      if (response&0x08)
         I_Error("SDReadSingleBlock: error response = 'Out of range' (sector: 0x%.8X)", sector);
      //if (response&0x10)
      //   I_Error("SDReadSingleBlock: error response = 'Card locked'");
   }*/

   return response;
}

int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
   if (numblocks == 0)
      return -1;

   uint32_t cursor = 0;

   uint8_t tmp[512];
   uint8_t checksum[2];

   //printf("SDReadMultipleBlocks: ptr:0x%.8X #:%d ba:0x%.8X\r\n", datablock, numblocks, blockaddress);
   for(uint32_t b=0; b<numblocks; ++b)
   {
      uint8_t response = SDReadSingleBlock(b+blockaddress, tmp, checksum);
      if (response != 0xFE)
         return -1;
      __builtin_memcpy(datablock+cursor, tmp, 512);
      cursor += 512;
   }

   return cursor;
}

int SDCardStartup()
{
   int k=0;
   uint8_t response[3];

   response[0] = SDIdle();
   if (response[0] != 0x01)
      return -1;

   uint32_t databack;
   response[1] = SDCheckVoltageRange(&databack);
   if (response[1] != 0x01)
      return -1;

   response[2] = SDCardInit();
   if (response[2] == 0x00) // OK
      return 0;

   if (response[2] != 0x01) // 0x05, older card, bail out
      return -1;
   
   // Keep looping until we get a 0x0
   while (response[2] == 0x01) // Loop again if it's 0x01
   {
      response[2] = SDCardInit();
      k=k^0xFF;
   } // Repeat this until we receive a non-idle (0x00)

   // NOTE: Block size is already set to 512 for high speed and can't be changed
   //response[3] = SDSetBlockSize512();
   //EchoUART("SDSetBlockSize512()");

   return response[2] == 0x00 ? 0 : -1;
}
