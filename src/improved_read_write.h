#ifndef IMPROVED_READ_WRITE_H_
#define IMPROVED_READ_WRITE_H_
/*
 * improved_read_write.h
 *
 *  Created on: march 2017
 *      Author: tyronevb
 */

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

// function declarations
int BigReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int BigWriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

#ifdef __cplusplus
}
#endif
#endif /* IMPROVED_READ_WRITE_ */
