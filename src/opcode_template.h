/*
 * opcode_template.h
 *
 *  Created on: 07 Jun 2016
 *      Author: tyronevb
 */

// include this .h file in all other headers of the project

#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"
#include "register.h"
#include "i2c_master.h"
#include "flash_sdram_controller.h"
#include "icape_controller.h"
#include "isp_spi_controller.h"
#include "one_wire.h"
#include "eth_mac.h"
#include "eth_sorter.h"
#include "custom_constants.h"
#include "invalid_nack.h"

#ifndef OPCODE_TEMPLATE_H_
#define OPCODE_TEMPLATE_H_

// function declarations
int NewOpCodeHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);


#endif /* OPCODE_TEMPLATE_H_ */
