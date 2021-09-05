#include "inttypes.h"

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

uint8_t SDIdle();
uint8_t SDCheckVoltageRange(uint32_t *databack);
uint8_t SDCardInit();
uint8_t SDSetBlockSize512();
uint8_t SDReadSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2]);
uint8_t SDWriteSingleBlock(uint32_t blockaddress, uint8_t *datablock, uint8_t checksum[2]);

int SDCardStartup();
int SDReadMultipleBlocks(uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);
int SDWriteMultipleBlocks(const uint8_t *datablock, uint32_t numblocks, uint32_t blockaddress);