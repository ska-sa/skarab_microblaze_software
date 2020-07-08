/*
 * LLDP.c
 *
 *  Created on: 28 Jun 2017
 *      Author: sinethemba
 */

/* standard C lib includes */
#include <string.h>

/* local includes */
#include "lldp.h"
#include "constant_defs.h"
#include "register.h"
#include "if.h"
#include "logging.h"

static char lookup[17] = "0123456789ABCDEF";

/***********************LLDP API functions***************************/
//=================================================================================
//  uLLDPBuildPacket
//---------------------------------------------------------------------------------
// This method builds the LLDP Packet into the user supplied buffer
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId     IN  Selected Ethernet interface
//  pTransmitBuffer   OUT Location to create packet
//  uResponseLength   OUT Length of packet created
//
//  Return
//  ------
//  None
//==================================================================================
int uLLDPBuildPacket(u8 uId, u8 *pTransmitBuffer, u32 *uResponseLength){
  struct sIFObject *pIF;
  int i;
  u8  uPaddingLength = 0;
  u32 uIPAddress;
  u32 version;
  u8 dst_mac_addr[ETH_DST_LEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };
  u8 *src_mac_addr;
  u8 *hostname;
  u16 first_part;

  pIF = lookup_if_handle_by_id(uId);

  uIPAddress = pIF->uIFAddrIP;
  src_mac_addr = pIF->arrIFAddrMac;

  /* zero the buffer, saves us from having to explicitly set zero valued bytes */
  memset(pTransmitBuffer, 0, LLDP_MAX_BUFFER_SIZE);

  /******* Ethernet frame stuff ******/

  /* fill in our hardware multicast mac address(destination address) */
  memcpy(pTransmitBuffer + ETH_DST_OFFSET, dst_mac_addr, ETH_DST_LEN);

  /* fill in our src hardware mac address */
  memcpy(pTransmitBuffer + ETH_SRC_OFFSET, src_mac_addr, ETH_SRC_LEN);

  /* ethernet frame type */
  pTransmitBuffer[ETH_FRAME_TYPE_OFFSET] = ((ETHERNET_TYPE_LLDP >> 8) & 0xFF);
  pTransmitBuffer[ETH_FRAME_TYPE_OFFSET + 1] = (ETHERNET_TYPE_LLDP & 0xFF);

  /* chassis ID TLV */
  pTransmitBuffer[LLDP_CHASSIS_ID_TLV_TYPE_OFFSET] = LLDP_CHASSIS_ID_TLV;
  pTransmitBuffer[LLDP_CHASSIS_ID_TLV_LEN_OFFSET] = LLDP_CHASSIS_ID_TLV_LEN;
  pTransmitBuffer[LLDP_CHASSIS_ID_TLV_OFFSET] = LLDP_CHASSIS_ID_MAC_ADDRESS;
  memcpy(pTransmitBuffer + LLDP_CHASSIS_ID_TLV_OFFSET + 1, src_mac_addr, 6);

  /* port ID TLV*/
  pTransmitBuffer[LLDP_PORT_ID_TLV_TYPE_OFFSET] =  LLPD_PORT_ID_TLV;
  pTransmitBuffer[LLDP_PORT_ID_TLV_LEN_OFFSET] =  LLDP_PORT_ID_TLV_LEN;
  pTransmitBuffer[LLDP_PORT_ID_TLV_OFFSET] = LLDP_PORT_ID_INTERFACE_NAME;


  /* Tome to live TLV */
  pTransmitBuffer[LLDP_TTL_TLV_TYPE_OFFSET] = LLDP_TTL_TLV;
  pTransmitBuffer[LLDP_TTL_TLV_LEN_OFFSET] = LLDP_TTL_TLV_LEN;
  pTransmitBuffer[LLDP_TTL_TLV_OFFSET] = 0;
  pTransmitBuffer[LLDP_TTL_TLV_OFFSET + 1] = 120;

  /* port description TLV */
  pTransmitBuffer[LLDP_PORT_DESCR_TLV_TYPE_OFFSET] = LLDP_PORT_DESC_TLV;
  pTransmitBuffer[LLDP_PORT_DESCR_TLV_LEN_OFFSET] = LLDP_PORT_DESCR_TLV_LEN;
  switch(uId){
    /*
     *  TODO: it would be more efficient to use the uId to generate the following named fields
     */
    case 0:
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-00", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_ONE_GBE_INTERFACE, LLDP_PORT_DESCR_TLV_LEN);
      break;

    case 1:
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-01", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_FORTY_GBE_INTERFACE_0, LLDP_PORT_DESCR_TLV_LEN);
      break;

    case 2:
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-02", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_FORTY_GBE_INTERFACE_1, LLDP_PORT_DESCR_TLV_LEN);
      break;

    case 3:
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-03", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_FORTY_GBE_INTERFACE_2, LLDP_PORT_DESCR_TLV_LEN);
      break;

    case 4:
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-04", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_FORTY_GBE_INTERFACE_3, LLDP_PORT_DESCR_TLV_LEN);
      break;

    /* max of 5 interfaces allowed in the architecture - 1x 1gbe + 4x 40gbe */
    default:
      /* execution path should never reach here but populate fields for debugging in case it does */
      memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, "I/F-xx", 6);
      memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET, LLDP_FORTY_GBE_INTERFACE_x, LLDP_PORT_DESCR_TLV_LEN);
      break;
  }

  /* System name  TLV */
  pTransmitBuffer[LLDP_SYSTEM_NAME_TLV_TYPE_OFFSET] = LLDP_SYSTEM_NAME_TLV;
  pTransmitBuffer[LLDP_SYSTEM_NAME_TLV_LEN_OFFSET] = LLDP_SYSTEM_NAME_TLV_LEN;

  hostname = if_generate_hostname_string(uId);
  /* NOTE: the if_generate_hostname_string() returns a string including the interface number appended to it but we ill
   * only copy the first part e.g. skarab020304 in the next line */
  memcpy(pTransmitBuffer + LLDP_SYSTEM_NAME_TLV_OFFSET, hostname, LLDP_SYSTEM_NAME_TLV_LEN);

  /* LLDP_SYSTEM_DESCR_TLV */
  pTransmitBuffer[LLDP_SYSTEM_DESCR_TLV_TYPE_OFFSET] = LLDP_SYSTEM_DESCR_TLV;
  pTransmitBuffer[LLDP_SYSTEM_DESCR_TLV_LEN_OFFSET] = LLDP_SYSTEM_DESCR_TLV_LEN;
  version = ReadBoardRegister(C_RD_VERSION_ADDR);
  first_part = (version & 0xff000000) >> 24; // for BSP or TLF
  if(first_part > 0){
    memcpy(pTransmitBuffer + LLDP_SYSTEM_DESCR_TLV_OFFSET, "BSP", LLDP_SYSTEM_DESCR_TLV_LEN);
  }
  else{
    memcpy(pTransmitBuffer + LLDP_SYSTEM_DESCR_TLV_OFFSET, "TLF", LLDP_SYSTEM_DESCR_TLV_LEN);
  }

  /* LLDP Management Address TLV */
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_TYPE_OFFSET] = LLDP_MANAGEMENT_ADDRESS_TLV;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_LEN_OFFSET] = LLDP_MANAGEMENT_ADDRESS_TLV_LEN;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET] = 5;    /* Length of Management Address + Management Address subtype */
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 1] = LLDP_MANAGEMENT_ADDRESS_IP_ADDRESS;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 2] =  (uIPAddress & 0xff000000) >> 24;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 3] =  (uIPAddress & 0x00ff0000) >> 16;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 4] =  (uIPAddress & 0x0000ff00) >> 8;
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 5] =  (uIPAddress & 0x000000ff);
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 6] = 2; // Interface numbering subtype
  pTransmitBuffer[LLDP_MANAGEMENT_ADDRESS_TLV_OFFSET + 7] = uId; // Interface number

  /* LLDP END OF LLDPDU TLV */
  pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET] = LLDP_END_OF_LLDPDU_TLV;
  pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET] = 0;

  *uResponseLength = LLDP_END_OF_LLDPDU_TLV_OFFSET + 1;

  /* pad to nearest 64 bit boundary */
  /* simply increase total length by following amount - these bytes already zero due to earlier memset */
  uPaddingLength = 8 - (*uResponseLength % 8);

  /* and with 0b111 since only interested in values of 0 to 7 */
  uPaddingLength = uPaddingLength & 0x7;

  *uResponseLength = *uResponseLength + uPaddingLength;

  log_printf(LOG_SELECT_LLDP, LOG_LEVEL_INFO, "LLDP [%02x] sending LLDP (len %d)\r\n", (pIF != NULL) ?
      pIF->uIFEthernetId : 0xFF, *uResponseLength);

  log_printf(LOG_SELECT_LLDP, LOG_LEVEL_TRACE, "LLDP tx pkt:");
  for (i = 0; i < *uResponseLength; i++){
    log_printf(LOG_SELECT_LLDP, LOG_LEVEL_TRACE, " %02x", pTransmitBuffer[i]);
  }
  log_printf(LOG_SELECT_LLDP, LOG_LEVEL_TRACE, "\r\n");

  return LLDP_RETURN_OK;

}

//======================================================================
//  reverse
//----------------------------------------------------------------------
//  This method reverses a 'str' of length 'len'
//       Parameter      Dir         Description
//       ---------      ---         -----------
//       str            IN/OUT      buffer with the string to be reverse
//       len            IN          len of the string in 'str'
//=====================================================================

void reverse(char *str, int len)
{
  int i=0, j=len-1, temp;
  while (i<j)
  {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++; j--;
  }
}


//==================================================================
//  u8ToStr
//------------------------------------------------------------------
//  This method converts a given u8 to *string
//       Parameter       Dir       Description
//       --------        ---       -----------
//       u8 x            IN        number to be converted
//       str             OUT       stores the string representation of x
//       d               IN        number of digits required in output

int u8ToStr(u8 x, char *str, int d)
{
  int i = 0;
  while (x)
  {
    str[i++] = lookup[(x%16)];
    x = x/16;
  }

  // If number of digits required is more, then
  // add 0s at the beginning
  while (i < d)
    str[i++] = '0';

  reverse(str, i);
  str[i] = '\0';
  return i;
}


