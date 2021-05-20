#include "SDCARD.h"
#include "utils.h"

uint8_t SDIdle()
{
   uint8_t response;

   *IO_SPIOutput = 0xFF;

   // Enter idle state
   *IO_SPIOutput = SPI_CMD(CMD0_GO_IDLE_STATE);
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x95; // Checksum
   int timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01

   return response;
}

uint8_t SDCheckVoltageRange(uint32_t &databack)
{
   uint8_t response;

   *IO_SPIOutput = 0xFF;

   *IO_SPIOutput = SPI_CMD(CMD8_SEND_IF_COND);
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x01;
   *IO_SPIOutput = 0xAA;
   *IO_SPIOutput = 0x87; // Checksum
   int timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card) - got 0x01

   // Read the 00 00 01 AA sequence back from the SD CARD
   databack = 0x00000000;
   *IO_SPIOutput = 0xFF;
   databack |= *IO_SPIInput;
   *IO_SPIOutput = 0xFF;
   databack |= (databack<<8)|(*IO_SPIInput);
   *IO_SPIOutput = 0xFF;
   databack |= (databack<<8)|(*IO_SPIInput);
   *IO_SPIOutput = 0xFF;
   databack |= (databack<<8)|(*IO_SPIInput);

   return response;
}

uint8_t SDCardInit()
{
   uint8_t response;

   *IO_SPIOutput = 0xFF;

   // ACMD header
   *IO_SPIOutput = SPI_CMD(CMD55_APP_CMD);
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00??

   // Set high capacity mode on
   *IO_SPIOutput = 0xFF;
   *IO_SPIOutput = SPI_CMD(ACMD41_SD_SEND_OP_COND);
   *IO_SPIOutput = 0x40;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0xFF; // checksum is not necessary at this point
   timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00 eventually, but will also get several 0x01 (idle)

   // Initialize
   /**IO_SPIOutput = 0xFF;
   *IO_SPIOutput = SPI_CMD(CMD1_SEND_OP_COND);
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0xFF; // checksum is not necessary at this point

   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
   } while(1); // Expected: 0x00*/

   return response;
}

uint8_t SDSetBlockSize512()
{
   uint8_t response;

   *IO_SPIOutput = 0xFF;

   // Set block length
   *IO_SPIOutput = SPI_CMD(CMD16_SET_BLOCKLEN);
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0x02;
   *IO_SPIOutput = 0x00;
   *IO_SPIOutput = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   return response;
}

uint8_t SDReadSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2])
{
   uint8_t response;

   *IO_SPIOutput = 0xFF;

   // Read single block
   *IO_SPIOutput = SPI_CMD(CMD17_READ_SINGLE_BLOCK);
   *IO_SPIOutput = (uint8_t)((blockaddress&0xFF000000)>>24);
   *IO_SPIOutput = (uint8_t)((blockaddress&0x00FF0000)>>16);
   *IO_SPIOutput = (uint8_t)((blockaddress&0x0000FF00)>>8);
   *IO_SPIOutput = (uint8_t)(blockaddress&0x000000FF);
   *IO_SPIOutput = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      *IO_SPIOutput = 0xFF;
      response = *IO_SPIInput;
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   if (response == 0x00)
   {
      timeout=65536;
      do {
         *IO_SPIOutput = 0xFF;
         response = *IO_SPIInput;
         if (response != 0xFF)
            break;
         --timeout;
      } while(timeout>0); // Expected: 0xFE

      if (response == 0xFE)
      {
         // Data burst follows
         // 512 bytes of data followed by 16 bit CRC, total of 514 bytes
         int x=0;
         do {
            *IO_SPIOutput = 0xFF;
            datablock[x++] = *IO_SPIInput;
         } while(x<512);

         // Checksum
         *IO_SPIOutput = 0xFF;
         checksum[0] = *IO_SPIInput;
         *IO_SPIOutput = 0xFF;
         checksum[1] = *IO_SPIInput;
      }
   }

   return response;
}

int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numbytes, uint32_t offset, uint32_t blockaddress)
{
   if (numbytes == 0)
      return -1;

   uint32_t topblock = offset/512;
   uint32_t bottomblock = (offset+numbytes)/512;
   uint32_t leftover = (offset+numbytes)%512;
   uint32_t cursor = 0;

   uint8_t tmp[512];
   uint8_t checksum[2];

   for(uint32_t b=topblock; b<=bottomblock; ++b)
   {
      uint8_t response = SDReadSingleBlock(b+blockaddress, tmp, checksum);
      if (response != 0xFE)
         return -1;

      if (b==topblock)
      {
         uint32_t copycount = (topblock==bottomblock) ? numbytes : (512-offset);
         __builtin_memcpy(datablock+cursor, &tmp[offset], copycount);
         cursor += copycount;
      }
      else if (b==bottomblock)
      {
         if (leftover)
         {
            __builtin_memcpy(datablock+cursor, tmp, leftover);
            cursor += leftover;
         }
      }
      else // Mid-blocks
      {
         __builtin_memcpy(datablock+cursor, tmp, 512);
         cursor += 512;
      }
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
   response[1] = SDCheckVoltageRange(databack);
   if (response[1] != 0x01)
      return -1;

   response[2] = SDCardInit();
   if (response[2] != 0x01)
      return -1;
   while (response[2] == 0x01)
   {
      response[2] = SDCardInit();
      k=k^0xFF;
   } // Repeat this until we receive a non-idle (0x00)

   // NOTE: Block size is already set to 512 for high speed and can't be changed
   //response[3] = SDSetBlockSize512();
   //EchoUART("SDSetBlockSize512()");

   return 0;
}
