/**----------------------------------------------------------------------------
 *   FILE:       arp.c
 *   BRIEF:      Implementation of ARP functionality.
 *
 *   DATE:       OCT 2017
 *
 *   COMPANY:    SKA SA
 *   AUTHOR:     R van Wyk
 *
 *   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
 *------------------------------------------------------------------------------*/

#include <xil_types.h>

#include "if.h"
#include "arp.h"
#include "print.h"

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_ARP(IFobject) if (SanityCheckARP(IFobject)) { return ARP_RETURN_FAIL; }
#else
#define SANE_ARP(IFobject)
#endif

#ifdef DO_SANITY_CHECKS
u8 SanityCheckARP(struct sIFObject *pIFObjectPtr){

  if (pIFObjectPtr == NULL){
    debug_printf("No interface state handle\r\n");
    return -1;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    debug_printf("Inconsistent interface state magic value\r\n");
    return -1;
  }

  if ((pIFObjectPtr->pUserTxBufferPtr) == NULL){
    debug_printf("Interface transmit buffer pointer undefined\r\n");
    return -1;
  }

  if ((pIFObjectPtr->pUserRxBufferPtr) == NULL){
    debug_printf("Interface receive buffer pointer undefined\r\n");
    return -1;
  }

  return 0;
}
#endif


/* ARP Reply Processing */
u8 uARPMessageValidateReply(struct sIFObject *pIFObjectPtr){
  u8 *pUserBufferPtr = NULL;

  const u8 uEthernetHWType[] = {0x0, 0x1};
  const u8 uIPProtocolType[] = {0x08, 0x00};
  const u8 uReplyOpcode[] = {0x00, 0x02};
  const u8 uRequestOpcode[] = {0x00, 0x01};

  SANE_ARP(pIFObjectPtr);

  pUserBufferPtr = pIFObjectPtr->pUserRxBufferPtr;

  if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_HW_TYPE_OFFSET, uEthernetHWType, 2) != 0){
    debug_printf("ARP: Ethernet HW Type problem!\r\n");
    return ARP_RETURN_INVALID;
  }

  if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_PROTO_TYPE_OFFSET, uIPProtocolType, 2) != 0){
    debug_printf("ARP: IPv4 Protocol Type problem!\r\n");
    return ARP_RETURN_INVALID;
  }

  /* NOTE: expecting IP over Ethernet ARP messages, thus hard code following lengths */
  /* ethernet length */
  if (pUserBufferPtr[ARP_FRAME_BASE + ARP_HW_ADDR_LENGTH_OFFSET] != 6){
    debug_printf("ARP: HW Addr length problem!!\r\n");
    return ARP_RETURN_INVALID;
  }

  /* ipv4 length */
  if (pUserBufferPtr[ARP_FRAME_BASE + ARP_PROTO_ADDR_LENGTH_OFFSET] != 4){
    debug_printf("ARP: Proto Addr length problem!!\r\n");
    return ARP_RETURN_INVALID;
  }

  /* are we the intended target of this arp packet? */
  if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_TGT_PROTO_ADDR_OFFSET, pIFObjectPtr->arrIFAddrIP, 4) != 0 ){
    trace_printf("ARP: ignore!\r\n");
    return ARP_RETURN_IGNORE;
  }

  /* is this an ARP reply? */
  if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_OPCODE_OFFSET, uReplyOpcode, 2) == 0){
    trace_printf("ARP: reply!\r\n");
    /* check for ip conflict between sender and us */
    if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_SRC_PROTO_ADDR_OFFSET, pIFObjectPtr->arrIFAddrIP, 4) == 0){
      trace_printf("ARP: address conflict!\r\n");
      return ARP_RETURN_CONFLICT;
    }
    return ARP_RETURN_REPLY;
  }

  /* is this an ARP request? */
  if (memcmp(pUserBufferPtr + ARP_FRAME_BASE + ARP_OPCODE_OFFSET, uRequestOpcode, 2) == 0){
    trace_printf("ARP: request!\r\n");
    return ARP_RETURN_REQUEST;
  }

  /* ignore / drop packet */
  return ARP_RETURN_IGNORE;
}


/* ARP Response */
u8 uARPBuildMessage(struct sIFObject *pIFObjectPtr, typeARPMessage tARPMsgType, u8 *arrTargetIP){
  u8 *pTxBuffer = NULL;
  u8 *pRxBuffer = NULL;
  u16 uSize; 
  u8 uIndex;

  SANE_ARP(pIFObjectPtr);

  pTxBuffer = pIFObjectPtr->pUserTxBufferPtr;
  pRxBuffer = pIFObjectPtr->pUserRxBufferPtr;

  uSize = pIFObjectPtr->uUserTxBufferSize;

  /* zero the buffer, saves us from having to explicitly set zero valued bytes */
  memset(pTxBuffer, 0, uSize);

  /*****  ethernet frame stuff  *****/ 

  /* if arp reply: unicast, else if request: broadcast */
  if (tARPMsgType == ARP_OPCODE_REPLY){
    /* copy destination addresses out of receive buffer */
    memcpy(pTxBuffer + ETH_DST_OFFSET, pRxBuffer + ETH_SRC_OFFSET, 6);
  } else if (ARP_OPCODE_REQUEST) {
    memset(pTxBuffer + ETH_DST_OFFSET, 0xff, 6);   /* broadcast */
  } else {
    /* unrecognized type */
    trace_printf("ARP: invalid message type!\r\n");
    return ARP_RETURN_FAIL;
  }

  /* fill in our hardware mac address */
  memcpy(pTxBuffer + ETH_SRC_OFFSET, pIFObjectPtr->arrIFAddrMac, 6);

  /* ethernet frame type arp = 0x0806 */
  pTxBuffer[ETH_FRAME_TYPE_OFFSET] = 0x08;
  pTxBuffer[ETH_FRAME_TYPE_OFFSET + 1] = 0x06;

  /*****  arp frame stuff  *****/ 

  //pTxBuffer[ARP_FRAME_BASE + ARP_HW_TYPE_OFFSET] = 0x00;
  pTxBuffer[ARP_FRAME_BASE + ARP_HW_TYPE_OFFSET + 1] = 0x01;  /* for ethernet */

  pTxBuffer[ARP_FRAME_BASE + ARP_PROTO_TYPE_OFFSET] = 0x08;   /* for ipv4 */
  /*already zero'ed */ //pTxBuffer[ARP_FRAME_BASE + ARP_PROTO_TYPE_OFFSET + 1] = 0x00;

  pTxBuffer[ARP_FRAME_BASE + ARP_HW_ADDR_LENGTH_OFFSET] = 6;
  pTxBuffer[ARP_FRAME_BASE + ARP_PROTO_ADDR_LENGTH_OFFSET] = 4;

  if (tARPMsgType == ARP_OPCODE_REPLY){
    pTxBuffer[ARP_FRAME_BASE + ARP_OPCODE_OFFSET + 1] = 2;
  } else if (ARP_OPCODE_REQUEST) {
    pTxBuffer[ARP_FRAME_BASE + ARP_OPCODE_OFFSET + 1] = 1;
  }

  memcpy(pTxBuffer + ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET, pIFObjectPtr->arrIFAddrMac, 6);
  memcpy(pTxBuffer + ARP_FRAME_BASE + ARP_SRC_PROTO_ADDR_OFFSET, pIFObjectPtr->arrIFAddrIP, 4);


  if (tARPMsgType == ARP_OPCODE_REPLY){
    /* THA ignored for requests */
    memcpy(pTxBuffer + ARP_FRAME_BASE + ARP_TGT_HW_ADDR_OFFSET, pRxBuffer + ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET, 6);
    memcpy(pTxBuffer + ARP_FRAME_BASE + ARP_TGT_PROTO_ADDR_OFFSET, pRxBuffer + ARP_FRAME_BASE + ARP_SRC_PROTO_ADDR_OFFSET, 4);
  }

  if (tARPMsgType == ARP_OPCODE_REQUEST){
    memcpy(pTxBuffer + ARP_FRAME_BASE + ARP_TGT_PROTO_ADDR_OFFSET, arrTargetIP, 4);
  }

  /* pad to 64 byites */
  /* simply increase total length by following amount - these bytes already zero due to earlier memset */
  /* in our case, arp packets are fixed length (14 + 28 = 42) */
  pIFObjectPtr->uMsgSize = ETH_FRAME_TOTAL_LEN + ARP_FRAME_TOTAL_LEN + 24;

  trace_printf("ARP  packet:\r\n");
  for (uIndex = 0; uIndex < pIFObjectPtr->uMsgSize; uIndex++){
    trace_printf("%02x ", pTxBuffer[uIndex]);
  }
  trace_printf("\r\n");

  return ARP_RETURN_OK;
}
