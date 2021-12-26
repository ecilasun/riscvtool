#include "core.h"
#include "spi.h"
#include "sdcard.h"
#include "uart.h"
#include <stdio.h>

#define G_SPI_TIMEOUT 32

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

// A single SPI transaction is a write from master followed by a read from slave's output
//volatile uint8_t *SPISINK = (volatile uint8_t* ) 0x8000000F;
uint8_t SPITx(const uint8_t outbyte)
{
   *IO_SPIRXTX = outbyte;
   //*SPISINK = 0xFF;
   //UARTWriteHexByte(outbyte);
   //UARTWrite("->");
   uint8_t incoming = *IO_SPIRXTX;
   //*SPISINK = 0xFF;
   //UARTWriteHexByte(incoming);
   //UARTWrite(":");
   return incoming;
}

uint8_t SDCmd(const SDCardCommand cmd, uint32_t args)
{
   uint8_t buf[8];

   buf[0] = SPI_CMD(cmd);
   buf[1] = (uint8_t)((args&0xFF000000)>>24);
   buf[2] = (uint8_t)((args&0x00FF0000)>>16);
   buf[3] = (uint8_t)((args&0x0000FF00)>>8);
   buf[4] = (uint8_t)(args&0x000000FF);
   buf[5] = CRC7(buf, 5);

   uint8_t incoming = SPITx(0xFF);
   for (uint32_t i=0;i<6;++i)
      incoming = SPITx(buf[i]);
   return incoming;
}

uint8_t SDIdle()
{
   uint8_t response;

   // Enter idle state
   SDCmd(CMD0_GO_IDLE_STATE, 0);

   int timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01

   return response;
}

uint8_t SDCheckVoltageRange(uint32_t *databack)
{
   uint8_t response;

   SDCmd(CMD8_SEND_IF_COND, 0x000001AA);

   int timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card) - got 0x01

   if (response != 0x01)
   {
      UARTWrite("Error version 2 SDCard expected\n");
      return response;
   }

   // Read the 00 00 01 AA sequence back from the SD CARD
   *databack = 0x00000000;
   *databack |= SPITx(0xFF);
   *databack |= (*databack<<8) | SPITx(0xFF);
   *databack |= (*databack<<8) | SPITx(0xFF);
   *databack |= (*databack<<8) | SPITx(0xFF);

   if (*databack != 0x000001AA)
   {
      UARTWrite("Expected 0x000001AA, got 0x");
      UARTWriteHex(*databack);
      UARTWrite("\n");
   }

   return response;
}

uint8_t SDCardInit()
{
   uint8_t response;

   // ACMD header
   SDCmd(CMD55_APP_CMD, 0x00000000);

   int timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00?? - NOTE: Won't handle old cards!

   // Set high capacity mode on
   SDCmd(ACMD41_SD_SEND_OP_COND, 0x40000000);

   timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00 eventually, but will also get several 0x01 (idle)

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

   return response;
}

uint8_t SDSetBlockSize512()
{
   uint8_t response;

   // Set block length
   SDCmd(CMD16_SET_BLOCKLEN, 0x00000200);

   int timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   return response;
}

uint8_t SDReadSingleBlock(uint32_t sector, uint8_t *datablock, uint8_t checksum[2])
{
   uint8_t response;

   // Read single block
   // NOTE: sector<<9 for non SDHC cards
   SDCmd(CMD17_READ_SINGLE_BLOCK, sector);

   // R1: expect 0x00
   int timeout=G_SPI_TIMEOUT;
   do {
      response = SPITx(0xFF);
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0);

   if (response != 0xFF) // == 0x00
   {
      // R2: expect 0xFE
      timeout=G_SPI_TIMEOUT;
      do {
         response = SPITx(0xFF);
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
            datablock[x++] = SPITx(0xFF);
         } while(x<512);

         // Checksum
         checksum[0] = SPITx(0xFF);
         checksum[1] = SPITx(0xFF);
      }
   }

   // Error
   if (!(response&0xF0))
   {
      if (response&0x01)
         UARTWrite("SDReadSingleBlock: error response = 'error'\n");
      if (response&0x02)
         UARTWrite("SDReadSingleBlock: error response = 'CC error'\n");
      if (response&0x04)
         UARTWrite("SDReadSingleBlock: error response = 'Card ECC failed'\n");
      if (response&0x08)
      {
         UARTWrite("SDReadSingleBlock: error response = 'Out of range' sector: 0x");
         UARTWriteHex(sector);
         UARTWrite("\n");
      }
      if (response&0x10)
         UARTWrite("SDReadSingleBlock: error response = 'Card locked'\n");
   }

   return response;
}

uint8_t SDWriteSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2])
{
   // TODO: 
   UARTWrite("SDWriteSingleBlock: not implemented yet.\n");

   return 0xFF;
}

int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
   if (numblocks == 0)
      return -1;

   uint32_t cursor = 0;

   uint8_t tmp[512];
   uint8_t checksum[2];

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

int SDWriteMultipleBlocks(const uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
   if (numblocks == 0)
      return -1;

   uint32_t cursor = 0;

   uint8_t tmp[512];
   uint8_t checksum[2];

   for(uint32_t b=0; b<numblocks; ++b)
   {
      __builtin_memcpy(tmp, datablock+cursor, 512);
      uint8_t response = SDWriteSingleBlock(b+blockaddress, tmp, checksum);
      if (response != 0xFE)
         return -1;
      cursor += 512;
   }

   return cursor;}

int SDCardStartup()
{
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
   } // Repeat this until we receive a non-idle (0x00)

   // NOTE: Block size is already set to 512 for high speed and can't be changed
   // Do I need to implement this one?
   //response[3] = SDSetBlockSize512();
   //EchoUART("SDSetBlockSize512()");

   int finalval = response[2] == 0x00 ? 0 : -1;

   return finalval;
}
