/*
 * improved_read_write.c
 *
 *  Created on: march 2017
 *      Author: tyronevb
 */

#include "improved_read_write.h"

// function definitions

//=================================================================================
//	ImprovedRead
//--------------------------------------------------------------------------------
//	This method executes the BIG_READ_REG command. Allows for reading more than 4 bytes
//  with a single transaction.
//  
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
int BigReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

	sBigReadWishboneReqT *Command = (sBigReadWishboneReqT *) pCommand;
	sBigReadWishboneRespT *Response = (sBigReadWishboneRespT *) uResponsePacketPtr;
	u32 uAddress;
	u32 uReadData;
	u16 uReadIndex;
	u16 uDataIndex = 0;
	u16 uNullWords;
	u16 uNullWordsIndex;

	if (uCommandLength < sizeof(sBigReadWishboneReqT)){
		return XST_FAILURE;
	}

	// Create response packet
	Response->Header.uCommandType = Command->Header.uCommandType + 1;
	Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
	Response->uStartAddressHigh = Command->uStartAddressHigh;
	Response->uStartAddressLow	= Command->uStartAddressLow;
	Response->uNumberOfReads = Command->uNumberOfReads;

	uAddress = (Command->uStartAddressHigh << 16) | (Command->uStartAddressLow);

	xil_printf("Response packet created OK!\n\r");

	// Execute the command
	for (uReadIndex = 0; uReadIndex < Command->uNumberOfReads; uReadIndex++)
	{
		// read 32-bit data
		uReadData = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress);
		Response->uReadData[uDataIndex] = (uReadData >> 16) & 0xFFFF; // upper 16 bits
		Response->uReadData[uDataIndex + 1] = uReadData & 0xFFFF; // lower 16 bits
		uDataIndex = uDataIndex + 2; // increment data index
		uAddress = uAddress + 4; // increment address by 4 to read the next 32-bit register
	}

	xil_printf("Big Read Completed OK!\n\r");
	xil_printf("Data High: %d\n\r", Response->uReadData[0]);
	xil_printf("Data Low: %d\n\r", Response->uReadData[1]);

	// determine how many null words are required to fill up the packet - don't need this. can just use the last uData
	xil_printf("Trying 994 words\n\r");
	uNullWords = 994 - (Command->uNumberOfReads)*2;

	for (uNullWordsIndex = Command->uNumberOfReads*2; uNullWordsIndex < uNullWords; uNullWordsIndex++)
	{
		Response->uReadData[uNullWordsIndex] = 0;
	}

	*uResponseLength = sizeof(sBigReadWishboneRespT);

	return XST_SUCCESS;
}
