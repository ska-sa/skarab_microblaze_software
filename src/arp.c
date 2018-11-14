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
#include <xil_assert.h>
#include <xil_io.h>

#include "eth_mac.h"
#include "if.h"
#include "arp.h"
#include "print.h"

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_ARP(IFobject) if (SanityCheckARP(IFobject)) { return ARP_RETURN_FAIL; }
/* TODO: abort() rather? */
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
u8 uARPBuildMessage(struct sIFObject *pIFObjectPtr, typeARPMessage tARPMsgType, u32 uTargetIP){
  u8 *pTxBuffer = NULL;
  u8 *pRxBuffer = NULL;
  u16 uSize; 
  u8 uIndex;
  u8 arrTargetIP[4] = {0};

  SANE_ARP(pIFObjectPtr);

  pTxBuffer = pIFObjectPtr->pUserTxBufferPtr;
  pRxBuffer = pIFObjectPtr->pUserRxBufferPtr;

  uSize = pIFObjectPtr->uUserTxBufferSize;

  if ((tARPMsgType != ARP_OPCODE_REPLY) && (tARPMsgType != ARP_OPCODE_REQUEST)){
    return ARP_RETURN_FAIL;
  }

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
    arrTargetIP[0] =  ((uTargetIP >> 24) & 0xff);
    arrTargetIP[1] =  ((uTargetIP >> 16) & 0xff);
    arrTargetIP[2] =  ((uTargetIP >>  8) & 0xff);
    arrTargetIP[3] =  ( uTargetIP        & 0xff);
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



//=================================================================================
//  ArpRequestHandler
//--------------------------------------------------------------------------------
//  This method constucts ARP requests to populate the ARP caches in the fabric
//  Ethernet interfaces.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  None
//=================================================================================
/* NOTE: FIXME? this implementation is limited to class C /24 networks since it only cycles
   through arp requests for IP's .1 to .254 (i.e. the last octet) */
void ArpRequestHandler(struct sIFObject *pIFObjectPtr)
{
  u32 RequestIP;
  int iStatus;
  u16 *pBuffer;
  u32 size, i;
  u8 id;

  pBuffer = (u16 *) pIFObjectPtr->pUserTxBufferPtr;
  if (NULL == pBuffer){
    Xil_AssertVoidAlways();
  }

  if (pIFObjectPtr->uIFEnableArpRequests == ARP_REQUESTS_ENABLE){
    RequestIP = pIFObjectPtr->uIFEthernetSubnet | pIFObjectPtr->uIFCurrentArpRequest;
    /* build the arp request */
    trace_printf("build : 0x%08x ", RequestIP);
    uARPBuildMessage(pIFObjectPtr, ARP_OPCODE_REQUEST, RequestIP);

    size = (u32) (pIFObjectPtr->uMsgSize >> 1);    /* bytes to 16-bit words */

    trace_printf("swap: %d ", size);
    for (i = 0; i < size; i++){
      pBuffer[i] = Xil_EndianSwap16(pBuffer[i]);
    }

    id = pIFObjectPtr->uIFEthernetId;

    size = size >> 1;   /*  32-bit words */
    trace_printf("send: %d %d ", size, id);
    iStatus = TransmitHostPacket(id, (u32 *) pBuffer, size);
    if (iStatus == XST_SUCCESS){
      pIFObjectPtr->uTxEthArpRequestOk++;
    } else {
      error_printf("ARP  [%02x] FAILED to send ARP REQUEST to target with IP 0x%08x\r\n", id, RequestIP);
      pIFObjectPtr->uTxEthArpErr++;
    }

    trace_printf("done\r\n");
    /* cycle through IP's from .1 to .254 */
    if (pIFObjectPtr->uIFCurrentArpRequest == 254){
      pIFObjectPtr->uIFCurrentArpRequest = 1;
    } else {
      pIFObjectPtr->uIFCurrentArpRequest++;
    }
  }
}
