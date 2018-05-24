/*
 * opcode_template.h
 *
 *  Created on: 07 Jun 2016
 *      Author: tyronevb
 */

#ifndef OPCODE_TEMPLATE_H_
#define OPCODE_TEMPLATE_H_

#include <xil_types.h>

// function declarations
int NewOpCodeHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

#endif /* OPCODE_TEMPLATE_H_ */
