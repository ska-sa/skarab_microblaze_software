/**---------------------------------------------------------------------------- 
*   FILE:       net_utils.h
*   BRIEF:      API of Network utility functions.
*   
*   DATE:       AUG 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   REFERENCES: RFC 791
*
*   [vim settings: "set sw=2 ts=2 expandtab autoindent"]
*------------------------------------------------------------------------------*/

#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_

/* some portability stuff */
/* for datatypes used */
#if !defined(u8) || !defined(u16) || !defined(u32)
#include <stdint.h>
#endif

#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

int uChecksum16Calc(u8 *pDataPtr, u16 uIndexStart, u16 uIndexEnd, u16 *pChecksumPtr, u8 ByteSwap, u16 uChecksumStartValue);
int uIPV4_ntoa(char *stringIP, u32 uIP32);

#endif /* _NET_UTILS_H_ */
