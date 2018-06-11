/**----------------------------------------------------------------------------
 *   FILE:       memtest.c
 *   BRIEF:      Implementation of the memory test algorithm for integrity testing.
 *
 *   DATE:       JAN 2018
 *
 *   COMPANY:    SKA SA
 *   AUTHOR:     R van Wyk
 *
 *   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
 *------------------------------------------------------------------------------*/

#include "memtest.h"
#include "print.h"

#define HIGHEST_16BIT_PRIME 16321

/* local function prototypes */
static inline u8 uAdler32Calc(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum);

//=================================================================================
//  uAdler32Calc
//---------------------------------------------------------------------------------
//  This method implements the Adler32 checksum algorithm over a given dataset.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pDataPtr        IN    pointer to the base of the dataset
//  uDataSize       IN    size of the dataset
//  pTxBufferPtr    OUT   ptr to user supplied 32bit data storage for calculated checksum
//
//  Return
//  ------
//  0 for success, -1 for error
//=================================================================================
static inline u8 uAdler32Calc(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum){
  u16 a = 1;
  u16 b = 0;

  u8 *pCurrentBytePtr = NULL;

  /* do the Adler32 calculation */
  for (pCurrentBytePtr = (u8 *) pDataPtr; pCurrentBytePtr < ((u8 *) pDataPtr + uDataSize); pCurrentBytePtr++){
    /* xil_printf("@%08p  0x%02x\r\n", pCurrentBytePtr, *pCurrentBytePtr); */
    a = a + (u16) *pCurrentBytePtr;
    b = b + a;
  }
  a = a % HIGHEST_16BIT_PRIME;
  b = b % HIGHEST_16BIT_PRIME;

  *pChecksum = (b << 16) | a;

  return 0;
}

//=================================================================================
//  uDoMemoryTest
//---------------------------------------------------------------------------------
//  This method is a wrapper around the checksum algorithm method.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pDataPtr        IN    pointer to the base of the dataset
//  uDataSize       IN    size of the dataset
//  pTxBufferPtr    OUT   ptr to user supplied 32bit data storage for calculated checksum
//
//  Return
//  ------
//  0 for success, -1 for error
//=================================================================================
u8 uDoMemoryTest(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum){
#ifdef DO_SANITY_CHECKS
  if (NULL == pDataPtr){
    debug_printf("[Memory Test] Error - No data reference\r\n");
    return -1;
  }

  if (0 == uDataSize){
    debug_printf("[Memory Test] Error - Zero data size\r\n");
    return -1;
  }

  if (NULL == pChecksum){
    debug_printf("[Memory Test] Error - No storage handle provided\r\n");
    return -1;
  }
#endif

  return uAdler32Calc(pDataPtr, uDataSize, pChecksum);
}
