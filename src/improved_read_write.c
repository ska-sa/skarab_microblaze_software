/*
 * improved_read_write.c
 *
 *  Created on: march 2017
 *      Author: tyronevb
 */

#include <xil_io.h>
#include <xstatus.h>

#include "improved_read_write.h"
#include "custom_constants.h"
#include "error.h"
#include "logging.h"
#include "register.h"

// function definitions

//=================================================================================
//  BigReadWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the BIG_READ_REG command. Allows for reading more than 4 bytes
//  with a single transaction.
//
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int BigReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

  sBigReadWishboneReqT *Command = (sBigReadWishboneReqT *) pCommand;
  sBigReadWishboneRespT *Response = (sBigReadWishboneRespT *) uResponsePacketPtr;
  u32 uAddress;
  u32 uReadData;
  u16 uReadIndex;
  u16 uDataIndex = 0;
  u16 uNullWords;
  u16 uNullWordsIndex;
  u8 errno = 0;

  if (uCommandLength < sizeof(sBigReadWishboneReqT)){
    return XST_FAILURE;
  }

  // Create response packet header
  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uStartAddressHigh = Command->uStartAddressHigh;
  Response->uStartAddressLow  = Command->uStartAddressLow;
  Response->uNumberOfReads = Command->uNumberOfReads;

  uAddress = (Command->uStartAddressHigh << 16) | (Command->uStartAddressLow);

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Response packet created OK!\r\n");

  // Execute the command
  for (uReadIndex = 0; uReadIndex < Command->uNumberOfReads; uReadIndex++)
  {
    // read 32-bit data
    /*uReadData = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress);*/
    uReadData = ReadWishboneRegister(uAddress);
    // place data in response packet as it gets read
    Response->uReadData[uDataIndex] = (uReadData >> 16) & 0xFFFF; // upper 16 bits
    Response->uReadData[uDataIndex + 1] = uReadData & 0xFFFF; // lower 16 bits
    uDataIndex = uDataIndex + 2; // increment data index
    uAddress = uAddress + 4; // increment address by 4 to read the next 32-bit register
  }

  errno = read_and_clear_error_flag();
  /* TODO: should we error out on all errno's or only on AXI_DATA_BUS errno? */
  if (errno == ERROR_AXI_DATA_BUS){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] AXI data bus error - wishbone addr outside range perhaps?\r\n", errno);
    /* overwrite uNumberOfReads field */
    Response->uNumberOfReads = BIG_WB_ERROR_MAGIC;    /* since the wishbone addr range timeout mechanism was added
                                                         later, a means to signal an error to casperfpga was required.
                                                         To maintain backward compatibility between casperfpga and
                                                         microblazes, a new field was not added and a magic number for
                                                         this field was decided upon. Also had no padding bytes to
                                                         repurpose for this use. */
  }

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Big Read Completed OK!\r\n");
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Data High: %d\r\n", Response->uReadData[0]);
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Data Low: %d\r\n", Response->uReadData[1]);

  // determine how many null words are required to fill up the packet
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Trying 994 words\r\n");
  uNullWords = 994 - (Command->uNumberOfReads)*2;

  for (uNullWordsIndex = Command->uNumberOfReads*2; uNullWordsIndex < uNullWords; uNullWordsIndex++)
  {
    Response->uReadData[uNullWordsIndex] = 0;
  }

  *uResponseLength = sizeof(sBigReadWishboneRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  BigWriteWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the BIG_WRITE_REG command. Allows for writing more than 4 bytes
//  with a single transaction.
//
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int BigWriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

  sBigWriteWishboneReqT *Command = (sBigWriteWishboneReqT *) pCommand;
  sBigWriteWishboneRespT *Response = (sBigWriteWishboneRespT *) uResponsePacketPtr;
  u32 uAddress;
  u32 uWriteData;
  u16 uWriteCount;
  u16 uDataIndex = 0;
  u16 uNumberOfWritesDone = 0;
  u8 uPaddingIndex;
  u8 errno = 0;

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Inside BigWriteWishboneCommandHandler now . . .\r\n");

  if (uCommandLength < sizeof(sBigWriteWishboneReqT)){
    return XST_FAILURE;
  }

  // determine the address at which to start
  uAddress = (Command->uStartAddressHigh << 16) | (Command->uStartAddressLow);

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Response packet created OK!\r\n");

  // Execute the write command(s)
  for (uWriteCount = 0; uWriteCount < Command->uNumberOfWrites; uWriteCount++)
  {
    // create 32-bit words from the 16-bit high, low words
    // requires the 32-bit data to be sent in the form: [WORD1: 16-bit high, WORD1: 16-bit low, WORD2: 16-bit high, WORD2: 16-bit low . . .]
    uWriteData = (Command->uWriteData[uDataIndex] << 16) | (Command->uWriteData[uDataIndex + 1]);

    // execute the write wishbone command
    /* Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress, uWriteData); */
    WriteWishboneRegister(uAddress, uWriteData);

    // debug msg
    //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Wrote data\r\n");

    uNumberOfWritesDone += 1;
    uDataIndex = uDataIndex + 2; // increment data index to get the next 32-bit word
    uAddress = uAddress + 4; // increment address by 4 to read the next 32-bit register
  }

  errno = read_and_clear_error_flag();
  /* TODO: should we error out on all errno's or only on AXI_DATA_BUS errno? */
  if (errno == ERROR_AXI_DATA_BUS){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] AXI data bus error - wishbone addr outside range perhaps?\r\n", errno);
    Response->uErrorStatus = 1;
  } else {
    Response->uErrorStatus = 0;
  }

  // debug msg
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Number of writes done: %d\r\n", uNumberOfWritesDone);
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Creating response packet. . .\r\n");

  // Create response packet
  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uStartAddressHigh = Command->uStartAddressHigh;
  Response->uStartAddressLow  = Command->uStartAddressLow;
  Response->uNumberOfWritesDone = uNumberOfWritesDone;

  // pad to minimum packet size and 64-bit boundary
  for (uPaddingIndex = 0; uPaddingIndex < 5; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sBigWriteWishboneRespT);

  return XST_SUCCESS;
}
