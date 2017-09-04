/**---------------------------------------------------------------------------- 
*   FILE:       net_utils.c
*   BRIEF:      Network utility functions.
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

#include <xil_types.h>

#include "net_utils.h"

//=================================================================================
//  uChecksum16Calc
//---------------------------------------------------------------------------------
//  This method calculates a 16-bit word checksum.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDataPtr        IN    pointer to buffer with data to be "checksummed"
//  uIndexStart     IN    starting index in the data buffer (zero indexed)
//  uIndexEnd       IN    ending index in the data buffer
//
//  Return
//  ------
//  0xffff or valid checksum  /*TODO FIXME 0xffff may also be a valid checksum!!!*/
//=================================================================================
u16 uChecksum16Calc(u8 *pDataPtr, u16 uIndexStart, u16 uIndexEnd){
  u16 uChkIndex;
  u16 uChkLength;
  u16 uChkTmp = 0;
  u32 uChecksumValue = 0;
  u32 uChecksumCarry = 0;

  if (pDataPtr == NULL){
    return 0xffff /* ? */;
  }

  /* range indexing error */
  if (uIndexStart > uIndexEnd){
    return 0xffff;  /* ? */
  }

  uChkLength = uIndexEnd - uIndexStart + 1;

  for (uChkIndex = uIndexStart; uChkIndex < (uIndexStart + uChkLength); uChkIndex += 2){
    uChkTmp = (u8) pDataPtr[uChkIndex];
    uChkTmp = uChkTmp << 8;
    if (uChkIndex == (uChkLength - 1)){     //last iteration - only valid for (uChkLength%2 == 1)
      uChkTmp = uChkTmp + 0;
    }
    else{
      uChkTmp = uChkTmp + (u8) pDataPtr[uChkIndex + 1];
    }

    /* get 1's complement of data */
    uChkTmp = ~uChkTmp;

    /* aggregate -> doing 16bit arithmetic within 32bit words to preserve overflow */
    uChecksumValue = uChecksumValue + uChkTmp;

    /* get overflow */
    uChecksumCarry = uChecksumValue >> 16;

    /* add to checksum */
    uChecksumValue = (0xffff & uChecksumValue) + uChecksumCarry;
    /* FIXME: repeat if this operation also produces an overflow!! */
  }

  return uChecksumValue;
}


//=================================================================================
//  uIPV4_ntoa
//---------------------------------------------------------------------------------
//  This method converts an IP address from a 32-bit word to a dotted notation string.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  stringIP        IN    pointer to a user provided string buffer (at least 16 bytes wide)
//                        where IP string will be stored.
//  uIP32           IN    IP address as a 32-bit unsigned integer.
//
//  Return
//  ------
//  0 for success and -1 for error
//=================================================================================
u8 uIPV4_ntoa(char *stringIP, u32 uIP32){
  u8 octet, digits, value, index;
  u8 i, j, k, x;

  if (stringIP == NULL){
    return -1;
  }

  index = 0;

  /* ipv4, therefore 4 octets */
  for (i = 0; i < 4; i++){
    /* get each octet of the 32-bit ip word */
    octet = (uIP32 >> (8 * (3 - i))) & 0xff;
    
    /* now convert octet to ascii representation: */

    /* how many digits in this octet? (base 10) 3, 2 or 1? */
    digits = (octet > 99) ? 3 : ((octet < 10) ? 1 : 2);

    /* for each of the digits in current octet, get the ascii value */
    for (j = digits; j > 0; j--){
      x = 1;
      /* calculate 10^(j-1) without math lib */
      for (k = 0; k < (j - 1); k++){
        x = x * 10;
      }
      value = octet / x;  /* get the relevant digit value */
      octet = octet % x;  /* now take the remainder for next iteration */
      stringIP[index] = value + 48; /* get ascii value */
      index = index + 1;
    }

    /* insert the dot for dotted notation and terminate string at the end */
    if (i == 3){
      stringIP[index++] = '\0';
    } else {
      stringIP[index++] = '.';
    }
  }

  return 0;
}
