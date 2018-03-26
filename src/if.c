/**----------------------------------------------------------------------------
*   FILE:       if.c
*   BRIEF:      Implementation for control the interface states.
*
*   DATE:       NOV 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
*------------------------------------------------------------------------------*/

#include <string.h>

#include "if.h"
#include "print.h"

u8 uInterfaceInit(struct sIFObject *pIFObjectPtr, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize, u8 *arrUserMacAddr, u8 uEthernetId){
  u8 uLoopIndex;

  if (pIFObjectPtr == NULL){
    return IF_RETURN_FAIL;
  }

  if (pRxBufferPtr == NULL){
    return IF_RETURN_FAIL;
  }

  if (pTxBufferPtr == NULL){
    return IF_RETURN_FAIL;
  }

  if ( arrUserMacAddr == NULL){
    return IF_RETURN_FAIL;
  }

  /* check that user supplied a reasonable amount of buffer space */
  if (uRxBufferSize < IF_RX_MIN_BUFFER_SIZE){
    return IF_RETURN_FAIL;
  }

  if (uTxBufferSize < IF_TX_MIN_BUFFER_SIZE){
    return IF_RETURN_FAIL;
  }

  pIFObjectPtr->pUserTxBufferPtr = pTxBufferPtr;
  pIFObjectPtr->uUserTxBufferSize = uTxBufferSize; /* bytes */

  pIFObjectPtr->pUserRxBufferPtr = pRxBufferPtr;
  pIFObjectPtr->uUserRxBufferSize = uRxBufferSize; /* bytes */

  /* zero the buffers */
  memset(pTxBufferPtr, 0x0, (size_t) uTxBufferSize);
  memset(pRxBufferPtr, 0x0, (size_t) uRxBufferSize);
  
  for (uLoopIndex = 0; uLoopIndex < 6; uLoopIndex++){
    pIFObjectPtr->arrIFAddrMac[uLoopIndex] = arrUserMacAddr[uLoopIndex];
    /* FIXME: possible buffer overrun if user issues smaller array - rather use string logic */
  }

  for (uLoopIndex = 0; uLoopIndex < 16; uLoopIndex++){
    pIFObjectPtr->stringIFAddrIP[uLoopIndex] = '\0';
    pIFObjectPtr->stringIFAddrNetmask[uLoopIndex] = '\0';
  }

  for (uLoopIndex = 0; uLoopIndex < 4; uLoopIndex++){
    pIFObjectPtr->arrIFAddrIP[uLoopIndex] = 0;
    pIFObjectPtr->arrIFAddrNetmask[uLoopIndex] = 0;
  }

  if (uDHCPInit(pIFObjectPtr) != DHCP_RETURN_OK){
    return IF_RETURN_FAIL;
  }

  pIFObjectPtr->uIFEthernetId = uEthernetId;
  pIFObjectPtr->uIFLinkStatus = LINK_DOWN;

  pIFObjectPtr->uMsgSize = 0;
  pIFObjectPtr->uNumWordsRead = 0;

  pIFObjectPtr->uRxTotal = 0;
  pIFObjectPtr->uRxEthArp = 0;
  pIFObjectPtr->uRxArpReply = 0;
  pIFObjectPtr->uRxArpRequest = 0;
  pIFObjectPtr->uRxArpConflict = 0;
  pIFObjectPtr->uRxArpInvalid = 0;
  pIFObjectPtr->uRxEthIp = 0;
  pIFObjectPtr->uRxIpChecksumErrors = 0;
  pIFObjectPtr->uRxIpIcmp = 0;
  pIFObjectPtr->uRxIcmpInvalid = 0;
  pIFObjectPtr->uRxIpUdp = 0;
  pIFObjectPtr->uRxUdpChecksumErrors = 0;
  pIFObjectPtr->uRxUdpCtrl = 0;
  pIFObjectPtr->uRxUdpDhcp = 0;
  pIFObjectPtr->uRxDhcpInvalid = 0;
  pIFObjectPtr->uRxDhcpUnknown = 0;
  pIFObjectPtr->uRxUdpUnknown = 0;
  pIFObjectPtr->uRxIpUnknown = 0;
  pIFObjectPtr->uRxEthUnknown = 0;

  pIFObjectPtr->uTxTotal = 0;
  pIFObjectPtr->uTxEthArpRequestOk = 0;
  pIFObjectPtr->uTxEthArpReplyOk = 0;
  pIFObjectPtr->uTxEthArpErr = 0;
  pIFObjectPtr->uTxEthLldpOk = 0;
  pIFObjectPtr->uTxEthLldpErr = 0;
  pIFObjectPtr->uTxIpIcmpReplyOk = 0;
  pIFObjectPtr->uTxIpIcmpReplyErr = 0;
  pIFObjectPtr->uTxIpIgmpOk = 0;
  pIFObjectPtr->uTxIpIgmpErr = 0;
  pIFObjectPtr->uTxUdpDhcpOk = 0;
  pIFObjectPtr->uTxUdpDhcpErr = 0;
  pIFObjectPtr->uTxUdpCtrlOk = 0;
  pIFObjectPtr->uTxUdpCtrlAck = 0;  /*TODO*/
  pIFObjectPtr->uTxUdpCtrlNack = 0; /*TODO*/

  pIFObjectPtr->uIFMagic = IF_MAGIC;

  return IF_RETURN_OK;
}
