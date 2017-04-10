
#ifndef IMPROVED_READ_WRITE_H_
#define IMPROVED_READ_WRITE_H_
/*
 * improved_read_write.h
 *
 *  Created on: march 2017
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

#include <stdbool.h>

// function declarations
int BigReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int BigWriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

#endif /* IMPROVED_READ_WRITE_ */
