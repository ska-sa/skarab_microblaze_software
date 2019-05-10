/*
 * invalid_nack.c
 *
 *  Created on: 10 May 2017
 *      Author: tyronevb
 */

#include <xil_types.h>
#include <xstatus.h>

#include "invalid_nack.h"
#include "custom_constants.h"
#include "logging.h"

// function definitions

//=================================================================================
//  InvalidOpcodeHandler
//--------------------------------------------------------------------------------
//  Function is called in the case where the SKARAB receives a request for an unknown opcode
//  Responds with a packet containing an opcode that indicates this behaviour
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
int InvalidOpcodeHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

  sInvalidOpcodeRespT *Response = (sInvalidOpcodeRespT *) uResponsePacketPtr;
  u8 uPaddingIndex;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Creating Response Packet for NACK. . .\r\n");
  // Create response packet
  Response->Header.uCommandType = NACK_OPCODE_RESP;
  Response->Header.uSequenceNumber = 0xFFFF;
  //Response->Dummy1 = 0;
  //Response->Dummy2 = 0;
  //Response->Dummy3 = 0;
  //Response->Dummy4 = 0;



  // padding bytes
  for (uPaddingIndex = 0; uPaddingIndex < 9; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "Created Response for NACK!!!!!\r\n");

  *uResponseLength = sizeof(sInvalidOpcodeRespT);

  return XST_SUCCESS;
}
