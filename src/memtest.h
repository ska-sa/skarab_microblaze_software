#ifndef _MEMTEST_H_
#define _MEMTEST_H_

#include "xil_types.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 uDoMemoryTest(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum);

#ifdef __cplusplus
}
#endif
#endif
