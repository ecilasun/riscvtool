#include "basesystem.h"
#include "spi.h"
#include "sdcard.h"
#include "uart.h"
#include <stdio.h>

typedef enum {
    CMD_NOT_SUPPORTED = -1,             /**< Command not supported error */
    CMD0_GO_IDLE_STATE = 0,             /**< Resets the SD Memory Card */
    CMD1_SEND_OP_COND = 1,              /**< Sends host capacity support */
    CMD6_SWITCH_FUNC = 6,               /**< Check and Switches card function */
    CMD8_SEND_IF_COND = 8,              /**< Supply voltage info */
    CMD9_SEND_CSD = 9,                  /**< Provides Card Specific data */
    CMD10_SEND_CID = 10,                /**< Provides Card Identification */
    CMD12_STOP_TRANSMISSION = 12,       /**< Forces the card to stop transmission */
    CMD13_SEND_STATUS = 13,             /**< Card responds with status */
    CMD16_SET_BLOCKLEN = 16,            /**< Length for SC card is set */
    CMD17_READ_SINGLE_BLOCK = 17,       /**< Read single block of data */
    CMD18_READ_MULTIPLE_BLOCK = 18,     /**< Card transfers data blocks to host until interrupted
                                                by a STOP_TRANSMISSION command */
    CMD24_WRITE_BLOCK = 24,             /**< Write single block of data */
    CMD25_WRITE_MULTIPLE_BLOCK = 25,    /**< Continuously writes blocks of data until
                                                'Stop Tran' token is sent */
    CMD27_PROGRAM_CSD = 27,             /**< Programming bits of CSD */
    CMD32_ERASE_WR_BLK_START_ADDR = 32, /**< Sets the address of the first write
                                                block to be erased. */
    CMD33_ERASE_WR_BLK_END_ADDR = 33,   /**< Sets the address of the last write
                                                block of the continuous range to be erased.*/
    CMD38_ERASE = 38,                   /**< Erases all previously selected write blocks */
    CMD55_APP_CMD = 55,                 /**< Extend to Applications specific commands */
    CMD56_GEN_CMD = 56,                 /**< General Purpose Command */
    CMD58_READ_OCR = 58,                /**< Read OCR register of card */
    CMD59_CRC_ON_OFF = 59,              /**< Turns the CRC option on or off*/
    // App Commands
    ACMD6_SET_BUS_WIDTH = 6,
    ACMD13_SD_STATUS = 13,
    ACMD22_SEND_NUM_WR_BLOCKS = 22,
    ACMD23_SET_WR_BLK_ERASE_COUNT = 23,
    ACMD41_SD_SEND_OP_COND = 41,
    ACMD42_SET_CLR_CARD_DETECT = 42,
    ACMD51_SEND_SCR = 51,
} SDCardCommand;

#define CMD8_PATTERN (0xAA)
#define SPI_CMD(x) (0x40 | (x & 0x3f))

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
uint8_t __attribute__ ((noinline)) SPITxRx(const uint8_t outbyte)
{
   *IO_SPIRXTX = outbyte;
   return *IO_SPIRXTX;
}

uint8_t __attribute__ ((noinline)) SDCmd(const SDCardCommand cmd, uint32_t args)
{
   uint8_t buf[8];

   buf[0] = SPI_CMD(cmd);
   buf[1] = (uint8_t)((args&0xFF000000)>>24);
   buf[2] = (uint8_t)((args&0x00FF0000)>>16);
   buf[3] = (uint8_t)((args&0x0000FF00)>>8);
   buf[4] = (uint8_t)(args&0x000000FF);
   buf[5] = CRC7(buf, 5);

   uint8_t incoming;

   SPITxRx(0xFF); // Dummy byte

   for (uint32_t i=0;i<6;++i)
      incoming = SPITxRx(buf[i]);

   return incoming;
}

uint8_t __attribute__ ((noinline)) SDResponse1()
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

uint8_t __attribute__ ((noinline)) SDResponse7(uint32_t *data)
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
uint8_t __attribute__ ((noinline)) SDIdle()
{
   E32Sleep(1); // Wait for at least 1ms after power-on

   // At least 74 cycles with CS low to go to SPI mode
   //*IO_SPICTL = 0x1; // CS high power low
   for (int i=0; i<80; ++i)
      SPITxRx(0xFF);
   //*IO_SPICTL = 0x3; // CS + power low (these signals are inverted)

   SDCmd(CMD0_GO_IDLE_STATE, 0);
   uint8_t response = SDResponse1(); // Expected: 0x01

   if (response != 0x01) // SPI mode
   {
      UARTWrite("SDIdle expected 0x01, got 0x");
      UARTWriteHex(response);
      UARTWrite("\r\n");
   }

   return response;
}

uint8_t __attribute__ ((noinline)) SDCheckVoltageRange()
{
   SDCmd(CMD8_SEND_IF_COND, 0x000001AA);

   // Read the response and 00 00 01 AA sequence back from the SD CARD
   uint32_t databack;
   uint8_t response = SDResponse7(&databack); // Expected: 0x01(version 2 SDCARD) or 0x05(version 1 or MMC card)

   if (response != 0x01) // V2 SD Card, 0x05 for a V1 SD Card / MMC card
   {
      UARTWrite("SDCheckVoltageRange expected 0x01, got 0x");
      UARTWriteHex(response);
      UARTWrite("\r\n");
   }

   if (databack != 0x000001AA)
   {
      UARTWrite("SDCheckVoltageRange expected 0x000001AA, got 0x");
      UARTWriteHex(databack);
      UARTWrite("\r\n");
   }

   return response;
}

uint8_t __attribute__ ((noinline)) SDCardInit()
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
      UARTWrite("\r\n");
   }

   if (rB != 0x00)
   {
      UARTWrite("SDCardInit expected 0x00, got 0x");
      UARTWriteHex(rB);
      UARTWrite("\r\n");
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

uint8_t __attribute__ ((noinline)) SDSetBlockSize512()
{
   // Set block length
   SDCmd(CMD16_SET_BLOCKLEN, 0x00000200);
   uint8_t response = SDResponse1();

   return response;
}

uint8_t __attribute__ ((noinline)) SDReadSingleBlock(uint32_t sector, uint8_t *datablock, uint8_t checksum[2])
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
         UARTWrite("SDReadSingleBlock: error response = 'error'\r\n");
      if (response&0x02)
         UARTWrite("SDReadSingleBlock: error response = 'CC error'\r\n");
      if (response&0x04)
         UARTWrite("SDReadSingleBlock: error response = 'Card ECC failed'\r\n");
      if (response&0x08)
      {
         UARTWrite("SDReadSingleBlock: error response = 'Out of range' sector: 0x");
         UARTWriteHex(sector);
         UARTWrite("\r\n");
      }
      if (response&0x10)
         UARTWrite("SDReadSingleBlock: error response = 'Card locked'\r\n");
   }

   return response;
}

uint8_t __attribute__ ((noinline)) SDWriteSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2])
{
   // TODO: CMD24_WRITE_BLOCK
   SDCmd(CMD17_READ_SINGLE_BLOCK, blockaddress);

   uint8_t response = SDResponse1(); // R1: expect 0x00

   if (response == 0x00) // SD_READY
   {
		// Send start token
		response = SPITxRx(0xFE);

         int x=0;
         do {
            response = SPITxRx(datablock[x++]);
         } while(x<512);

		response = SDResponse1(); // R1: status, expected status&x1F==0x05

		if ((response&0x1F) == 0x05) // 010:accepted, 101:rejected-crcerror, 110:rejected-writeerror
		{
			// Wait for write to finish
			response = SDResponse1();
		}
		else
		{
			UARTWrite("SDWriteSingleBlock: write error\r\n");
			return 0xFF;
		}
   }

   return response;
}

int __attribute__ ((noinline)) SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
	if (numblocks == 0)
		return -1;

	uint32_t cursor = 0;

	uint8_t checksum[2];

	for(uint32_t b=0; b<numblocks; ++b)
	{
      uint8_t* target = (uint8_t*)(datablock+cursor);
		uint8_t response = SDReadSingleBlock(b+blockaddress, target, checksum);
		if (response != 0xFE)
			return -1;
		cursor += 512;
	}

	return cursor;
}

int __attribute__ ((noinline)) SDWriteMultipleBlocks(const uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress)
{
	if (numblocks == 0)
		return -1;

	uint32_t cursor = 0;

	uint8_t checksum[2];

	for(uint32_t b=0; b<numblocks; ++b)
	{
      uint8_t* source = (uint8_t*)(datablock+cursor);
		uint8_t response = SDWriteSingleBlock(b+blockaddress, source, checksum);
		if (response != 0xFE)
			return -1;
		cursor += 512;
	}

	return cursor;
}

/*void __attribute__ ((noinline)) SDCardControl(int power_cs_n)
{
   *IO_SPICTL = power_cs_n;
}*/

int __attribute__ ((noinline)) SDCardStartup()
{
   uint8_t response[3];

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
