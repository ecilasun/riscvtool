#include "SDCARD.h"
#include "utils.h"

uint8_t SDIdle()
{
   uint8_t response;
   // Enter idle state
   SPIOutput[33] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD0_GO_IDLE_STATE);
   SPIOutput[1] = 0x00;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x00;
   SPIOutput[4] = 0x00;
   SPIOutput[5] = 0x95; // Checksum
   int timeout=65536;
   do {
      SPIOutput[36] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01

   return response;
}

uint8_t SDCheckVoltageRange(uint32_t &databack)
{
   uint8_t response;

   SPIOutput[35] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD8_SEND_IF_COND);
   SPIOutput[1] = 0x00;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x01;
   SPIOutput[4] = 0xAA;
   SPIOutput[5] = 0x87; // Checksum
   int timeout=65536;
   do {
      SPIOutput[36] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card) - got 0x01

   // Read the 00 00 01 AA sequence back from the SD CARD
   databack = 0x00000000;
   SPIOutput[36] = 0xFF;
   databack |= SPIInput[0];
   SPIOutput[36] = 0xFF;
   databack |= (databack<<8)|SPIInput[1];
   SPIOutput[36] = 0xFF;
   databack |= (databack<<8)|SPIInput[2];
   SPIOutput[36] = 0xFF;
   databack |= (databack<<8)|SPIInput[3];

   return response;
}

uint8_t SDCardInit()
{
   uint8_t response;

   // ACMD header
   SPIOutput[37] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD55_APP_CMD);
   SPIOutput[1] = 0x00;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x00;
   SPIOutput[4] = 0x00;
   SPIOutput[5] = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      SPIOutput[38] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00??

   // Set high capacity mode on
   SPIOutput[37] = 0xFF;
   SPIOutput[0] = SPI_CMD(ACMD41_SD_SEND_OP_COND);
   SPIOutput[1] = 0x40;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x00;
   SPIOutput[4] = 0x00;
   SPIOutput[5] = 0xFF; // checksum is not necessary at this point
   timeout=65536;
   do {
      SPIOutput[38] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00 eventually, but will also get several 0x01 (idle)

   // Initialize
   /*SPIOutput[37] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD1_SEND_OP_COND);
   SPIOutput[1] = 0x00;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x00;
   SPIOutput[4] = 0x00;
   SPIOutput[5] = 0xFF; // checksum is not necessary at this point

   do {
      SPIOutput[38] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
   } while(1); // Expected: 0x00*/

   return response;
}

uint8_t SDSetBlockSize512()
{
   uint8_t response;
   // Enter idle state
   SPIOutput[33] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD16_SET_BLOCKLEN);
   SPIOutput[1] = 0x00;
   SPIOutput[2] = 0x00;
   SPIOutput[3] = 0x02;
   SPIOutput[4] = 0x00;
   SPIOutput[5] = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      SPIOutput[36] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   return response;
}

uint8_t SDReadSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2])
{
   uint8_t response;
   // Enter idle state
   SPIOutput[33] = 0xFF;
   SPIOutput[0] = SPI_CMD(CMD17_READ_SINGLE_BLOCK);
   SPIOutput[1] = (uint8_t)((blockaddress&0xFF000000)>>24);
   SPIOutput[2] = (uint8_t)((blockaddress&0x00FF0000)>>16);
   SPIOutput[3] = (uint8_t)((blockaddress&0x0000FF00)>>8);
   SPIOutput[4] = (uint8_t)(blockaddress&0x000000FF);
   SPIOutput[5] = 0xFF; // checksum is not necessary at this point
   int timeout=65536;
   do {
      SPIOutput[36] = 0xFF;
      response = SPIInput[0];
      if (response != 0xFF)
         break;
      --timeout;
   } while(timeout>0); // Expected: 0x00

   if (response == 0x00)
   {
      timeout=65536;
      do {
         SPIOutput[36] = 0xFF;
         response = SPIInput[0];
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
            SPIOutput[36] = 0xFF;
            datablock[x++] = SPIInput[0];
         } while(x<512);

         // Checksum
         SPIOutput[36] = 0xFF;
         checksum[0] = SPIInput[0];
         SPIOutput[36] = 0xFF;
         checksum[1] = SPIInput[0];
      }
   }

   return response;
}

int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
   if (numblocks == 0)
      return -1;

   uint8_t checksum[2];
   for (uint32_t i=0;i<numblocks;++i)
   {
      uint8_t response = SDReadSingleBlock(blockaddress+i, datablock+512*i, checksum);
      /*for (uint32_t o=0;o<512;++o)
         EchoInt(((uint32_t*)(blockaddress+i))[o]);*/
      if (response != 0xFE)
         return -1;
   }
   return 1;
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
   return 0;
}
