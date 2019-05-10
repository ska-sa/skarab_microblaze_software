#include <xil_types.h>

#include "icmp.h"
#include "net_utils.h"
#include "constant_defs.h"
#include "logging.h"

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_ICMP(IFobject) if (SanityCheckICMP(IFobject)) { return ICMP_RETURN_FAIL; }
#else
#define SANE_ICMP(IFobject)
#endif

#ifdef DO_SANITY_CHECKS
u8 SanityCheckICMP(struct sIFObject *pIFObjectPtr){

  if (pIFObjectPtr == NULL){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ALWAYS, "No interface state handle\r\n");
    return -1;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ALWAYS, "Inconsistent interface state magic value\r\n");
    return -1;
  }

  if ((pIFObjectPtr->pUserTxBufferPtr) == NULL){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ALWAYS, "Interface transmit buffer pointer undefined\r\n");
    return -1;
  }

  if ((pIFObjectPtr->pUserRxBufferPtr) == NULL){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ALWAYS, "Interface receive buffer pointer undefined\r\n");
    return -1;
  }

  return 0;
}
#endif

/* Can eliminate the following function since these variables will move up to the interface object layer */
/* No states that need to be maintained - we respond immediately to a request */
#if 0
//=================================================================================
//  uICMPInit
//---------------------------------------------------------------------------------
//  Call this method to initialize the ICMP state object.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pICMPObjectPtr  IN    handle to ICMP state object
//  pRxBufferPtr    IN    ptr to user supplied rx buffer which will contain rx ICMP packet
//  uRxBufferSize   IN    size of the user specified rx buffer
//  pTxBufferPtr    OUT   ptr to user supplied tx buffer where ICMP message will be "built up"
//  uRxBufferSize   IN    size of the user specified tx buffer
//
//  Return
//  ------
// ICMP_RETURN_OK or ICMP_RETURN_FAIL 
//=================================================================================
u8 uICMPInit(struct sICMPObject *pICMPObjectPtr, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize){

  if (pICMPObjectPtr == NULL){
    return ICMP_RETURN_FAIL;
  }

  if (pRxBufferPtr == NULL){
    return ICMP_RETURN_FAIL;
  }

  if (pTxBufferPtr == NULL){
    return ICMP_RETURN_FAIL;
  }

  pICMPObjectPtr->pUserRxBufferPtr = pRxBufferPtr;
  pICMPObjectPtr->uUserRxBufferSize = uRxBufferSize;

  pICMPObjectPtr->pUserTxBufferPtr = pTxBufferPtr;
  pICMPObjectPtr->uUserTxBufferSize = uTxBufferSize;

  memset(pTxBufferPtr, 0x0, (size_t) uTxBufferSize);

  pICMPObjectPtr->uICMPMsgSize = 0;

  pICMPObjectPtr->uICMPMagic = ICMP_MAGIC;

  return ICMP_RETURN_OK;
}
#endif

//=================================================================================
//  uICMPMessageValidate
//---------------------------------------------------------------------------------
//  This method checks whether a message in the user rx buffer is a valid ICMP message
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr  IN    handle to IF state object
//
//  Return
//  ------
//  ICMP_RETURN_OK or ICMP_RETURN_FAIL or ICMP_RETURN_INVALID
//=================================================================================
u8 uICMPMessageValidate(struct sIFObject *pIFObjectPtr){
  u8 uIPLenAdjust;
  u8 *pUserBufferPtr;
  u16 uCheckTemp = 0;
  u16 uICMPTotalLength = 0;
  int RetVal = -1;

  SANE_ICMP(pIFObjectPtr);

  pUserBufferPtr = pIFObjectPtr->pUserRxBufferPtr;

  /* TODO: check for buffer overruns */

  /*adjust ip base value if ip length greater than 20 bytes*/
  uIPLenAdjust = (((pUserBufferPtr[IP_FRAME_BASE] & 0x0F) * 4) - 20);
  if (uIPLenAdjust > 40){
    return ICMP_RETURN_INVALID;
  }

  if (pUserBufferPtr[uIPLenAdjust + ICMP_FRAME_BASE + ICMP_TYPE_OFFSET] != 8){
    return ICMP_RETURN_INVALID;
  }

  if (pUserBufferPtr[uIPLenAdjust + ICMP_FRAME_BASE + ICMP_CODE_OFFSET] != 0){
    return ICMP_RETURN_INVALID;
  }

#if 0 /* NOW HANDLED BY LOWER LAYER */
  /* IP checksum validation*/
  RetVal = uChecksum16Calc(pUserBufferPtr, IP_FRAME_BASE, IP_FRAME_BASE + IP_FRAME_TOTAL_LEN + uIPLenAdjust - 1, &uCheckTemp, 0, 0);
  if(RetVal){
    return ICMP_RETURN_FAIL;
  }

  if (uCheckTemp != 0xFFFF){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ERROR, "ICMP: ECHO REQ - IP Hdr Checksum %04x - Invalid!\r\n", uCheckTemp);
    return ICMP_RETURN_INVALID;
  }
#endif

  /* ICMP total length = IP payload length = IP total length - IP Header length */
  uICMPTotalLength = (pUserBufferPtr[IP_FRAME_BASE + IP_TLEN_OFFSET] << 8) & 0xFF00;
  uICMPTotalLength |= (pUserBufferPtr[IP_FRAME_BASE + IP_TLEN_OFFSET + 1]) & 0x00FF;
  uICMPTotalLength = uICMPTotalLength - 20 - uIPLenAdjust;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: Length = %d\r\n", uICMPTotalLength);

  /* now check the ICMP checksum */
  RetVal = uChecksum16Calc(pUserBufferPtr, ICMP_FRAME_BASE, ICMP_FRAME_BASE + uICMPTotalLength - 1, &uCheckTemp, 0, 0);
  if(RetVal){
    return ICMP_RETURN_FAIL;
  }

  if (uCheckTemp != 0xFFFF){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ERROR, "ICMP: ECHO REQ - ICMP Checksum %04x - Invalid!\r\n", uCheckTemp);
    return ICMP_RETURN_INVALID;
  }

  return ICMP_RETURN_OK;   /* valid echo request */
}


//=================================================================================
//  uICMPBuildReplyMessage
//---------------------------------------------------------------------------------
//  This method builds the ICMP Reply Message.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pICMPObjectPtr  IN    handle to ICMP state object
//
//  Return
//  ------
//  ICMP_RETURN_OK or ICMP_RETURN_FAIL
//=================================================================================
u8 uICMPBuildReplyMessage(struct sIFObject *pIFObjectPtr){
  u8 *pTxBuffer;
  u8 *pRxBuffer;
  u8  uPaddingLength = 0;
  u16 uSize;
  u16 uIndex;
  u16 uICMPTotalLength;
  u16 uIPTotalLength;
  u16 uIPRxHdrLength;
  u16 uIPLenAdjust;
  u16 uChecksum;
  //int RetVal = (-1);

  SANE_ICMP(pIFObjectPtr);

  /* simplify ur life */
  pTxBuffer = pIFObjectPtr->pUserTxBufferPtr;
  pRxBuffer = pIFObjectPtr->pUserRxBufferPtr;

  uSize = pIFObjectPtr->uUserTxBufferSize;
  uIPTotalLength = (pRxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET] << 8) & 0xFF00;
  uIPTotalLength |= (pRxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET + 1]) & 0x00FF;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: RX IP Total Len %d\r\n", uIPTotalLength);

  /* zero the buffer, saves us from having to explicitly set zero valued bytes */
  memset(pTxBuffer, 0, uSize);

  /*****  ethernet frame stuff  *****/ 

  memcpy(pTxBuffer + ETH_DST_OFFSET, pRxBuffer + ETH_SRC_OFFSET, 6);
  memcpy(pTxBuffer + ETH_SRC_OFFSET, pRxBuffer + ETH_DST_OFFSET, 6);

  /* ethernet frame type */
  pTxBuffer[ETH_FRAME_TYPE_OFFSET] = 0x08;

  /*****  ip frame struff  *****/

  pTxBuffer[IP_FRAME_BASE + IP_FLAG_FRAG_OFFSET] = 0x40;    /* ip flags = don't fragment */
  pTxBuffer[IP_FRAME_BASE + IP_V_HIL_OFFSET] = 0x45;

  pTxBuffer[IP_FRAME_BASE + IP_TTL_OFFSET] = 0x80;
  pTxBuffer[IP_FRAME_BASE + IP_PROT_OFFSET] = 0x01;

  memcpy(pTxBuffer + IP_FRAME_BASE + IP_DST_OFFSET, pRxBuffer + IP_FRAME_BASE + IP_SRC_OFFSET, 4);
  memcpy(pTxBuffer + IP_FRAME_BASE + IP_SRC_OFFSET, pRxBuffer + IP_FRAME_BASE + IP_DST_OFFSET, 4);

  /*****  icmp frame struff  *****/
  pTxBuffer[ICMP_FRAME_BASE + ICMP_TYPE_OFFSET] = 0;

  pTxBuffer[ICMP_FRAME_BASE + ICMP_CODE_OFFSET] = 0;

  pTxBuffer[ICMP_FRAME_BASE + ICMP_CHKSM_OFFSET] = 0;
  pTxBuffer[ICMP_FRAME_BASE + ICMP_CHKSM_OFFSET + 1] = 0;

  /* calculate the packet lengths */
  uIPTotalLength = (pRxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET] << 8) & 0xFF00;
  uIPTotalLength |= (pRxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET + 1]) & 0x00FF;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: RX IP Total Len %d\r\n", uIPTotalLength);

  uIPRxHdrLength = (pRxBuffer[IP_FRAME_BASE] & 0x0F) * 4;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: RX IP Hdr Len %d\r\n", uIPRxHdrLength);

  /* adjust ip base value if ip length greater than 20 bytes */
  uIPLenAdjust = uIPRxHdrLength - 20;

  uICMPTotalLength = uIPTotalLength - uIPRxHdrLength;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: ICMP Total Len %d\r\n", uICMPTotalLength);

  for (uIndex = 0; uIndex < (uICMPTotalLength - 4); uIndex++){  /* ICMP header length = 4 */
    /* copy ICMP payload */
    pTxBuffer[ICMP_FRAME_BASE + ICMP_DATA_OFFSET + uIndex] = pRxBuffer[ICMP_FRAME_BASE + ICMP_DATA_OFFSET + uIndex + uIPLenAdjust];
  }

  /* calculate ICMP checksum */
  uChecksum16Calc(pTxBuffer, ICMP_FRAME_BASE, ICMP_FRAME_BASE + uICMPTotalLength - 1, &uChecksum, 0, 0);
  pTxBuffer[ICMP_FRAME_BASE + ICMP_CHKSM_OFFSET    ] = (u8) ((uChecksum & 0xff00) >> 8);
  pTxBuffer[ICMP_FRAME_BASE + ICMP_CHKSM_OFFSET + 1] = (u8) (uChecksum & 0xff);

  /* calculate and fill in ip frame packet lengths */
  uIPTotalLength = uICMPTotalLength + IP_FRAME_TOTAL_LEN;
  pTxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET    ] = (u8) ((uIPTotalLength & 0xff00) >> 8);
  pTxBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET + 1] = (u8) (uIPTotalLength & 0xff);

  /* calculate IP checksum */
  uChecksum16Calc(pTxBuffer, IP_FRAME_BASE, IP_FRAME_BASE + IP_FRAME_TOTAL_LEN - 1, &uChecksum, 0, 0);
  pTxBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET    ] = (u8) ((uChecksum & 0xff00) >> 8);
  pTxBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET + 1] = (u8) (uChecksum & 0xff);

  /* pad to nearest 64 bit boundary */
  /* simply increase total length by following amount - these bytes already zero due to earlier memset */
  uPaddingLength = 8 - ((uIPTotalLength + ETH_FRAME_TOTAL_LEN) % 8);

  /* and with 0b111 since only interested in values of 0 to 7 */
  uPaddingLength = uPaddingLength & 0x7; 

  pIFObjectPtr->uMsgSize = uIPTotalLength + ETH_FRAME_TOTAL_LEN + uPaddingLength;

  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: RX IP Hdr Len %d\r\n", uIPRxHdrLength);
  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: RX IP Total Len %d\r\n", uIPTotalLength);
  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: ICMP Total Len %d\r\n", uICMPTotalLength);

#if 0 /* tx buffer can now be traced at a lower level */
  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "ICMP: packet\r\n");
  for (uIndex=ICMP_FRAME_BASE; uIndex < ICMP_FRAME_BASE + uICMPTotalLength; uIndex++){
    log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, " %02x", pTxBuffer[uIndex]);
  }
  log_printf(LOG_SELECT_ICMP, LOG_LEVEL_TRACE, "\r\n");
#endif

  return ICMP_RETURN_OK;
}
