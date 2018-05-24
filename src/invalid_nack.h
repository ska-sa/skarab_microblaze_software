/*
 * invalid_nack.h
 *
 *  Created on: 10 May 2017
 *      Author: tyronevb
 */
#ifndef INVALID_NACK_H_
#define INVALID_NACK_H_

#include <xil_types.h>

// function declarations
int InvalidOpcodeHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

#endif /* INVALID_NACK_H_ */
