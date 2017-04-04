/*
 * opcode_template.c
 *
 *  Created on: 07 Jun 2016
 *      Author: tyronevb
 */

#include "opcode_template.h"

// function definitions

//=================================================================================
//	NewOpCodeHandler
//--------------------------------------------------------------------------------
//	Implement a new opcode of the uBlaze host control API using this skeleton
//  Do not change the parameters of the function
//  The structures sNewOpCodeReqT and sNewOpCodeRespT should be defined in
//  constant_defs.h
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int NewOpCodeHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

	sNewOpCodeReqT *Command = (sNewOpCodeReqT *) pCommand;
	sNewOpCodeRespT *Response = (sNewOpCodeRespT *) uResponsePacketPtr;
	u8 uPaddingIndex;

	if (uCommandLength < sizeof(sNewOpCodeReqT)){
		return XST_FAILURE;
	}

	// Execute the command

	// statements to perform the function of the opcode here



	// Create response packet
	Response->Header.uCommandType = Command->Header.uCommandType + 1;
	Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

	// padding bytes
	for (uPaddingIndex = 0; uPaddingIndex < 3; uPaddingIndex++)
		Response->uPadding[uPaddingIndex] = 0;

	*uResponseLength = sizeof(sNewOpCodeRespT);

	return XST_SUCCESS;
}
