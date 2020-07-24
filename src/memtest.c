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
#include "logging.h"
#include "delay.h"

#define HIGHEST_16BIT_PRIME 16321

/* local function prototypes */
static inline u8 uAdler32Calc(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum);
static u8 uDoMemoryTest(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum);

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
    /* log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "@%08p  0x%02x\r\n", pCurrentBytePtr, *pCurrentBytePtr); */
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
static u8 uDoMemoryTest(const u8 *pDataPtr, const u32 uDataSize, u32 *pChecksum){
#ifdef DO_SANITY_CHECKS
  if (NULL == pDataPtr){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "[Memory Test] Error - No data reference\r\n");
    return -1;
  }

  if (0 == uDataSize){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "[Memory Test] Error - Zero data size\r\n");
    return -1;
  }

  if (NULL == pChecksum){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "[Memory Test] Error - No storage handle provided\r\n");
    return -1;
  }
#endif

  return uAdler32Calc(pDataPtr, uDataSize, pChecksum);
}



//=================================================================================
//  vRunMemoryTest
//---------------------------------------------------------------------------------
//  Run the memory test
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  void
//
//  Return
//  ------
//  void
//=================================================================================

/* this variable holds the end address of the .text section. See linker script */
extern u32 _text_section_end_;
/* this variable holds the location at which the externally computed checksum was stored by linker. See linker script */
extern u32 _location_checksum_;

void vRunMemoryTest(void){
  u32 uMemTest = 0;

  /* NOTE: &_text_section_end_ gives us the address of the last program 32bit word
     but we're looking for size in bytes - therefore add 3 to include lower 3 bytes as well
     and add another one to prevent off-by-one error*/
  if (0 == uDoMemoryTest((u8 *) 0x50, (((u32) &_text_section_end_) + 3 - 0x50 + 1), &uMemTest)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "[Memory Test] from addr @0x%08x to @0x%08x...\r\n"\
                                 "[Memory Test] expected value {@0x%08x}: 0x%08x\r\n"\
                                 "[Memory Test] computed value              : 0x%08x\r\n",\
                                 0x50, &_text_section_end_, &_location_checksum_,\
                                 _location_checksum_, uMemTest);
    /* do not allow failure to go unnoticed - Assert()! */
    if (uMemTest != _location_checksum_){
      while(1){
        /* enter endless printout loop in order to debug with serial console */
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS,  "[Memory Test] ***FAILED*** expect: 0x%08x compute: 0x%08x\r\n", _location_checksum_, uMemTest);
        Delay(1000000);
      }
    } else {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "[Memory Test] ***PASSED***\r\n");
    }
  } else {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "[Memory Test] Error - could not execute\r\n");
  }
}
