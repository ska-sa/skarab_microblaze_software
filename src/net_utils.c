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
#include <xenv_standalone.h>

#include "net_utils.h"
#include "eth.h"
#include "ipv4.h"
#include "dhcp.h"

//=================================================================================
//  uChecksum16Calc
//---------------------------------------------------------------------------------
//  This method calculates a 16-bit word checksum.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pDataPtr        IN    pointer to buffer with data to be "checksummed"
//  uIndexStart     IN    starting index in the data buffer (zero indexed)
//  uIndexEnd       IN    ending index in the data buffer
//  pChecksumPtr    OUT   pointer to a buffer to store calculated checksum
//
//  Return
//  ------
//  0 for success and -1 for error
//=================================================================================
int uChecksum16Calc(u8 *pDataPtr, u16 uIndexStart, u16 uIndexEnd, u16 *pChecksumPtr, u8 ByteSwap, u16 uChecksumStartValue){
  u16 uChkIndex;
  u16 uChkLength;
  u16 uChkTmp = 0;
  u32 uChecksumValue = 0;
  u32 uChecksumCarry = 0;
  u8 Offset = 0;

  if (pDataPtr == NULL){
    return (-1);
  }

  /* range indexing error */
  if (uIndexStart > uIndexEnd){
    return (-1);
  }

  if (ByteSwap){
    Offset = 1;
  } else {
    Offset = 0;
  }

  uChecksumValue = ( (u32) uChecksumStartValue) & 0x0000FFFF;

  uChkLength = uIndexEnd - uIndexStart + 1;

  for (uChkIndex = uIndexStart; uChkIndex < (uIndexStart + uChkLength); uChkIndex += 2){
    uChkTmp = (u8) pDataPtr[uChkIndex + Offset];
    uChkTmp = uChkTmp << 8;
    if (uChkIndex == (uIndexStart + uChkLength - 1)){     //last iteration - only valid for (uChkLength%2 == 1)
      uChkTmp = uChkTmp + 0;
    }
    else{
      uChkTmp = uChkTmp + (u8) pDataPtr[uChkIndex + 1 - Offset];
    }

    /* get 1's complement of data */
    uChkTmp = ~uChkTmp;

    /* aggregate -> doing 16bit arithmetic within 32bit words to preserve overflow */
    uChecksumValue = uChecksumValue + uChkTmp;

    /* get overflow */
    uChecksumCarry = (uChecksumValue >> 16) & 0xffff;

    /* add to checksum */
    uChecksumValue = (0xffff & uChecksumValue) + uChecksumCarry;
  }

  *pChecksumPtr = uChecksumValue;

  return 0;
}

//***********************************************************************************
/* TODO: function description */
//***********************************************************************************

int uUDPChecksumCalc(u8 *pDataPtr, u16 *pChecksumPtr){
  u8 uPseudoHdr[12] = {0};
  u8 uIPLenAdjust = 0;
  u16 uCheckTemp = 0;
  u16 uUDPLength = 0;

  uIPLenAdjust = (((pDataPtr[IP_FRAME_BASE] & 0x0F) * 4) - 20);
  if (uIPLenAdjust > 40){
    return (-1);
  }

  /* build the pseudo IP header for UDP checksum calculation */
  memcpy(&(uPseudoHdr[0]), pDataPtr + IP_FRAME_BASE + IP_SRC_OFFSET, 4);
  memcpy(&(uPseudoHdr[4]), pDataPtr + IP_FRAME_BASE + IP_DST_OFFSET, 4);
  uPseudoHdr[8] = 0;
  uPseudoHdr[9] = pDataPtr[IP_FRAME_BASE + IP_PROT_OFFSET];
  memcpy(&(uPseudoHdr[10]), pDataPtr + uIPLenAdjust + UDP_FRAME_BASE + UDP_ULEN_OFFSET, 2);

  if(uChecksum16Calc(&(uPseudoHdr[0]), 0, 11, &uCheckTemp, 0, 0)){
    return (-1);
  }

  uUDPLength = (pDataPtr[uIPLenAdjust + UDP_FRAME_BASE + UDP_ULEN_OFFSET] << 8) & 0xFF00;
  uUDPLength |= ((pDataPtr[uIPLenAdjust + UDP_FRAME_BASE + UDP_ULEN_OFFSET + 1] ) & 0x00FF);

  /* UDP checksum validation*/
  if(uChecksum16Calc(pDataPtr, uIPLenAdjust + UDP_FRAME_BASE, uIPLenAdjust + UDP_FRAME_BASE + uUDPLength - 1, &uCheckTemp, 0, uCheckTemp)){
    return (-1);
  }

  *pChecksumPtr = uCheckTemp;

  return 0;

}

//***********************************************************************************
/* TODO: function description */
//***********************************************************************************

int uIPChecksumCalc(u8 *pDataPtr, u16 *pChecksumPtr){
  u8 uIPLenAdjust = 0;
  u16 uCheckTemp = 0;

  uIPLenAdjust = (((pDataPtr[IP_FRAME_BASE] & 0x0F) * 4) - 20);
  if (uIPLenAdjust > 40){
    return (-1);
  }

  if(uChecksum16Calc(pDataPtr, IP_FRAME_BASE, UDP_FRAME_BASE + uIPLenAdjust - 1, &uCheckTemp, 0, 0)){
    return (-1);
  }

  *pChecksumPtr = uCheckTemp;

  return 0;
}

//=================================================================================
//  uIPV4_ntoa
//---------------------------------------------------------------------------------
//  This method converts an IP address from a 32-bit word to a dotted notation string.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  stringIP        IN    pointer to a user provided string buffer (at least 16 bytes wide)
//                        where IP string will be stored.
//  uIP32           IN    IP address as a 32-bit unsigned integer.
//
//  Return
//  ------
//  0 for success and -1 for error
//=================================================================================
int uIPV4_ntoa(char *stringIP, u32 uIP32){
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
