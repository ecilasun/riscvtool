#include "core.h"
#include "spi.h"
#include "sdcard.h"
#include "uart.h"
#include <stdio.h>

#define G_SPI_TIMEOUT 65536

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
uint8_t SPITxRx(const uint8_t outbyte)
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

   SPITxRx(0xFF); // Dummy

   uint8_t incoming;
   for (uint32_t i=0;i<6;++i)
      incoming = SPITxRx(buf[i]);

   SPITxRx(0xFF); // Dummy

   return incoming;
}

uint8_t SDResponse1()
{
   uint8_t res1 = 0xFF;

   int timeout = 0;
   while((res1 = SPITxRx(0xFF)) == 0xFF) {
      ++timeout;
      if (timeout > G_SPI_TIMEOUT)
         break;
   };

   return res1;
}

uint8_t SDResponse7(uint32_t *data)
{
   *data = 0x00000000;
   uint8_t res1 = SDResponse1();
   if (res1 > 1) return 0xFF;

   uint8_t d[4];
   d[0] = SPITxRx(0xFF);
   d[1] = SPITxRx(0xFF);
   d[2] = SPITxRx(0xFF);
   d[3] = SPITxRx(0xFF);

   *data = (d[0]<<24) | (d[1]<<16) | (d[2]<<8) | (d[3]);

   return res1;
}

// Enter idle state
uint8_t SDIdle()
{
   SDCmd(CMD0_GO_IDLE_STATE, 0);
   uint8_t response = SDResponse1(); // Expected: 0x01

   if (response != 0x01)
   {
      UARTWrite("SDIdle expected 0x01, got 0x");
      UARTWriteHex(response);
      UARTWrite("\n");
   }

   return response;
}

uint8_t SDCheckVoltageRange()
{
   SDCmd(CMD8_SEND_IF_COND, 0x000001AA);

   // Read the response and 00 00 01 AA sequence back from the SD CARD
   uint32_t databack;
   uint8_t response = SDResponse7(&databack); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card)

   if (response != 0x01) // V2 SD Card, 0x05 for a V1 SD Card / MMC card
   {
      UARTWrite("SDCheckVoltageRange expected 0x01, got 0x");
      UARTWriteHex(response);
      UARTWrite("\n");
   }

   if (databack != 0x000001AA)
   {
      UARTWrite("SDCheckVoltageRange expected 0x000001AA, got 0x");
      UARTWriteHex(databack);
      UARTWrite("\n");
   }

   return response;
}

uint8_t SDCardInit()
{
   // Set high capacity mode on
   int timeout = 0;
   uint8_t rA = 0xFF, rB = 0xFF;
   do {
      // ACMD header
      SDCmd(CMD55_APP_CMD, 0x00000000);
      rA = SDResponse1(); // Expected: 0x00?? - NOTE: Won't handle old cards!
      SDCmd(ACMD41_SD_SEND_OP_COND, 0x40000000);
      rB = SDResponse1(); // Expected: 0x00 eventually, but will also get several 0x01 (idle)
      ++timeout;
   } while (rB != 0x00 && timeout < G_SPI_TIMEOUT);

   if (rA != 0x01)
   {
      UARTWrite("SDCardInit expected 0x01, got 0x");
      UARTWriteHex(rA);
      UARTWrite("\n");
   }

   if (rB != 0x00)
   {
      UARTWrite("SDCardInit expected 0x00, got 0x");
      UARTWriteHex(rB);
      UARTWrite("\n");
   }

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

   return rB;
}

uint8_t SDSetBlockSize512()
{
   // Set block length
   SDCmd(CMD16_SET_BLOCKLEN, 0x00000200);
   uint8_t response = SDResponse1();
   return response;
}

uint8_t SDReadSingleBlock(uint32_t sector, uint8_t *datablock, uint8_t checksum[2])
{
   // Read single block
   // NOTE: sector<<9 for non SDHC cards
   SDCmd(CMD17_READ_SINGLE_BLOCK, sector);
   uint8_t response = SDResponse1(); // R1: expect 0x00

   if (response != 0xFF) // == 0x00
   {
      response = SDResponse1(); // R2: expect 0xFE

      if (response == 0xFE)
      {
         // Data burst follows
         // 512 bytes of data followed by 16 bit CRC, total of 514 bytes
         int x=0;
         do {
            datablock[x++] = SPITxRx(0xFF);
         } while(x<512);

         // Checksum
         checksum[0] = SPITxRx(0xFF);
         checksum[1] = SPITxRx(0xFF);
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

   return cursor;
}

int SDCardStartup()
{
   uint8_t response[3];

   // At least 75 bits worth of idling around before we start
   // Here we send 8x10=80 bits worth of dummy bytes
   for (uint32_t i=0;i<10;++i)
      SPITxRx(0xFF);

   response[0] = SDIdle();
   if (response[0] != 0x01)
      return -1;

   response[1] = SDCheckVoltageRange();
   if (response[1] != 0x01)
      return -1;

   response[2] = SDCardInit();
   if (response[2] == 0x00) // OK
      return 0;
   
   return -1;

   // NOTE: Block size is already set to 512 for high speed and can't be changed
   // Do I need to implement this one?
   //response[3] = SDSetBlockSize512();
   //EchoUART("SDSetBlockSize512()");
}
