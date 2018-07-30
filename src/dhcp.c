/**----------------------------------------------------------------------------
 *   FILE:       dhcp.c
 *   BRIEF:      Implementation of DHCP client functionality to obtain and
 *               renew IPv4 leases.
 *
 *   DATE:       MAY 2017
 *
 *   COMPANY:    SKA SA
 *   AUTHOR:     R van Wyk
 *
 *   REFERENCES: RFC 2131 & 2132
 *------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

/* standard C lib includes */
//#include <stdio.h>
//#include <string.h>

#include <xil_types.h>
#include <xil_printf.h>
#include <xenv_standalone.h>

/* local includes */
#include "dhcp.h"
#include "eth.h"
#include "ipv4.h"
#include "udp.h"
#include "net_utils.h"
#include "if.h"
#include "print.h"

/* Local function prototypes  */
static typeDHCPState init_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState randomize_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState select_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState wait_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState request_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState bound_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState renew_dhcp_state(struct sIFObject *pIFObjectPtr);
static typeDHCPState rebind_dhcp_state(struct sIFObject *pIFObjectPtr);

static u8 uDHCPBuildMessage(struct sIFObject *pIFObjectPtr, typeDHCPMessage tDHCPMsgType, typeDHCPMessageFlags tDHCPMsgFlags);
static typeDHCPMessage tDHCPProcessMsg(struct sIFObject *pIFObjectPtr);

static u8 uDHCPWait(struct sIFObject *pIFObjectPtr);
static u32 uDHCPRandomNumber(u32 uSeed);

static void vDHCPAuxClearFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition);
static void vDHCPAuxSetFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition);
static typeDHCPFlagStatus tDHCPAuxTestFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition);

/* some global declarations */
static dhcp_state_func_ptr dhcp_state_table[] = {
  [INIT]      = init_dhcp_state,
  [RANDOMIZE] = randomize_dhcp_state,
  [SELECT]    = select_dhcp_state,
  [WAIT]      = wait_dhcp_state,
  [REQUEST]   = request_dhcp_state,
  [BOUND]     = bound_dhcp_state,
  [RENEW]     = renew_dhcp_state,
  [REBIND]    = rebind_dhcp_state };

static const char *dhcp_msg_string_lookup[] = {
  [DHCPDISCOVER]  = "DISCOVER",
  [DHCPOFFER]     = "OFFER",
  [DHCPREQUEST]   = "REQUEST",
  [DHCPDECLINE]   = "DECLINE",
  [DHCPACK]       = "ACK",
  [DHCPNAK]       = "NAK",
  [DHCPRELEASE]   = "RELEASE",
  [DHCPINFORM]    = "INFORM"
};

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_DHCP(IFobject) if (SanityCheckDHCP(IFobject)) { return DHCP_RETURN_FAIL; }
#else
#define SANE_DHCP(IFobject)
#endif

#ifdef DO_SANITY_CHECKS
u8 SanityCheckDHCP(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  /* TODO: should we assert rather that return? */

  if (pIFObjectPtr == NULL){
    debug_printf("No interface state handle\r\n");
    return -1;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    debug_printf("Inconsistent interface state magic value\r\n");
    return -1;
  }

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* This next test is strictly speaking not necessary since this piece of memory is allocated when the IFObject struct is defined */
  /* and the pointer to it should thus not be NULL. */
  /* However, for possible future implementations where this could be decoupled and initialized seperately, keep the code here */
  /* but comment out. */
#if 0
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }
#endif

  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    debug_printf("Inconsistent DHCP state magic value\r\n");
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

/*********** DHCP API functions **********/

//=================================================================================
//  uDHCPInit
//---------------------------------------------------------------------------------
//  Call this method to initialize the DHCP state object. This method must be called
//  first before invoking the DHCP state machine.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  pRxBufferPtr    IN    ptr to user supplied rx buffer which will contain rx dhcp packet
//  uRxBufferSize   IN    size of the user specified rx buffer
//  pTxBufferPtr    OUT   ptr to user supplied tx buffer where dhcp message will be "built up"
//  uRxBufferSize   IN    size of the user specified tx buffer
//
//  Return
//  ------
// DHCP_RETURN_OK or DHCP_RETURN_FAIL 
//=================================================================================
u8 uDHCPInit(struct sIFObject *pIFObjectPtr){
  u8 uLoopIndex;
  struct sDHCPObject *pDHCPObjectPtr;

#ifdef DO_SANITY_CHECKS
  /* do not call SANE_DHCP macro function since magic values will not yet be set */
  /* macro guard this check separately */
  if (pIFObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }
#endif

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* strictly this check not necessary - see comment above */
#if 0
#ifdef DO_SANITY_CHECKS
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }
#endif
#endif

  pDHCPObjectPtr->tDHCPCurrentState = INIT;

  pDHCPObjectPtr->uDHCPMessageReady = 0;
  pDHCPObjectPtr->uDHCPTx = 0;
  pDHCPObjectPtr->uDHCPRx = 0;
  pDHCPObjectPtr->uDHCPErrors = 0;
  pDHCPObjectPtr->uDHCPInvalid = 0;
  pDHCPObjectPtr->uDHCPRetries = 0;

  pDHCPObjectPtr->uDHCPRebootReqs = 0;

  pDHCPObjectPtr->uDHCPTimeout = 0;
  pDHCPObjectPtr->uDHCPTimeoutStatus = 0;

  pDHCPObjectPtr->uDHCPInternalTimer = 0;
  pDHCPObjectPtr->uDHCPCurrentClkTick = 0;
  pDHCPObjectPtr->uDHCPCachedClkTick = 0;
  pDHCPObjectPtr->uDHCPExternalTimerTick = 0;
  pDHCPObjectPtr->uDHCPRandomWait = 0;
  pDHCPObjectPtr->uDHCPRandomWaitCached = (u32) uDHCPWait(pIFObjectPtr);

  /* assign default values, can be overriden by API functions */
  pDHCPObjectPtr->uDHCPSMRetryInterval = DHCP_SM_INTERVAL;
  pDHCPObjectPtr->uDHCPSMInitWait = DHCP_SM_WAIT;

  pDHCPObjectPtr->uDHCPT1 = 0;
  pDHCPObjectPtr->uDHCPT2 = 0;
  pDHCPObjectPtr->uDHCPLeaseTime = 0;

  for (uLoopIndex = 0; uLoopIndex < 6; uLoopIndex++){
    pDHCPObjectPtr->arrDHCPNextHopMacCached[uLoopIndex] = 0xff;
  }

  for (uLoopIndex = 0; uLoopIndex < 4; uLoopIndex++){
    pDHCPObjectPtr->arrDHCPAddrYIPCached[uLoopIndex] = 0;
    pDHCPObjectPtr->arrDHCPAddrServerCached[uLoopIndex] = 0;
    pDHCPObjectPtr->arrDHCPAddrSubnetMask[uLoopIndex] = 0;
    pDHCPObjectPtr->arrDHCPAddrRoute[uLoopIndex] = 0;
  }

  for (uLoopIndex = 0; uLoopIndex < HOST_NAME_MAX_SIZE; uLoopIndex++){
    pDHCPObjectPtr->arrDHCPHostName[uLoopIndex] = '\0';
  }

  pDHCPObjectPtr->callbackOnLeaseAcqd = NULL;
  pDHCPObjectPtr->pLeaseDataPtr = NULL;
  pDHCPObjectPtr->callbackOnMsgBuilt = NULL;
  pDHCPObjectPtr->pMsgDataPtr = NULL;

  pDHCPObjectPtr->uDHCPXidCached = 0;
  pDHCPObjectPtr->uDHCPRegisterFlags = 0;

  pDHCPObjectPtr->uDHCPMagic = DHCP_MAGIC;

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPMessageValidate
//---------------------------------------------------------------------------------
//  This method checks whether a message in the user rx buffer is valid for the
//  current DHCP context / state. Use in conjuction with uDHCPSetGotMsgFlag().
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL or DHCP_RETURN_INVALID
//=================================================================================
u8 uDHCPMessageValidate(struct sIFObject *pIFObjectPtr){
  //u8 uBootpPort[] = {0x00, 0x44};
  u8 arrDHCPCookie[] = {0x63, 0x82, 0x53, 0x63};
  u8 uIPLen;
  u8 uIndex;
  u8 *pUserBufferPtr;
  //u16 uCheckTemp = 0;
  //u8 uPseudoHdr[12] = {0};
  //u16 uUDPLength = 0;
  //int RetVal = -1;
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  pUserBufferPtr = pIFObjectPtr->pUserRxBufferPtr;

  /* TODO: check for buffer overruns */

  /* do the quicker checks first! */

  /*adjust ip base value if ip length greater than 20 bytes*/
  uIPLen = (((pUserBufferPtr[IP_FRAME_BASE] & 0x0F) * 4) - 20);
  if (uIPLen > 40){
    return DHCP_RETURN_INVALID;
  }

  if (memcmp(pUserBufferPtr + uIPLen + DHCP_OPTIONS_BASE, arrDHCPCookie, 4) != 0){                 /*dhcp magic cookie?*/
    return DHCP_RETURN_INVALID;
  }

  /* check message-xid against cached-xid */
  for (uIndex = 0; uIndex < 4; uIndex++){
    if (pUserBufferPtr[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + uIndex] != (((pDHCPObjectPtr->uDHCPXidCached) >> (8 * (3 - uIndex))) & 0xff)){
      return DHCP_RETURN_INVALID;
    }
  }

#if 0 /* NOW HANDLED BY LOWER LAYER */
  /* is the port number correct for a bootp/dhcp reply */
  if (memcmp(pUserBufferPtr + uIPLen + UDP_FRAME_BASE + UDP_DST_PORT_OFFSET, uBootpPort, 2) != 0){   /*port 68?*/
    return DHCP_RETURN_INVALID;
  }
#endif

  if (pUserBufferPtr[uIPLen + BOOTP_FRAME_BASE + BOOTP_OPTYPE_OFFSET] != 0x02){                  /*bootp reply?*/
    return DHCP_RETURN_INVALID;
  }

#if 0 /* NOW HANDLED BY LOWER LAYER */
  /* IP checksum validation*/
  RetVal = uChecksum16Calc(pUserBufferPtr, IP_FRAME_BASE, UDP_FRAME_BASE + uIPLen - 1, &uCheckTemp, 0, 0);
  if(RetVal){
    return DHCP_RETURN_FAIL;
  }

  if (uCheckTemp != 0xFFFF){
    xil_printf("DHCP: RX - IP Hdr Checksum %04x - Invalid!\r\n", uCheckTemp);
    return DHCP_RETURN_INVALID;
  }

  /* is this a UDP packet? */
  if (pUserBufferPtr[IP_FRAME_BASE + IP_PROT_OFFSET] != 0x11){
    return DHCP_RETURN_INVALID; /* not a udp packet */
  }

  /* now check the UDP checksum */

  /* build the pseudo IP header for UDP checksum calculation */
  memcpy(&(uPseudoHdr[0]), pUserBufferPtr + IP_FRAME_BASE + IP_SRC_OFFSET, 4);
  memcpy(&(uPseudoHdr[4]), pUserBufferPtr + IP_FRAME_BASE + IP_DST_OFFSET, 4);
  uPseudoHdr[8] = 0;
  uPseudoHdr[9] = pUserBufferPtr[IP_FRAME_BASE + IP_PROT_OFFSET];
  memcpy(&(uPseudoHdr[10]), pUserBufferPtr + uIPLen + UDP_FRAME_BASE + UDP_ULEN_OFFSET, 2);

  RetVal = uChecksum16Calc(&(uPseudoHdr[0]), 0, 11, &uCheckTemp, 0, 0);
  if(RetVal){
    return DHCP_RETURN_FAIL;
  }

  uUDPLength = (pUserBufferPtr[uIPLen + UDP_FRAME_BASE + UDP_ULEN_OFFSET] << 8) & 0xFF00;
  uUDPLength |= ((pUserBufferPtr[uIPLen + UDP_FRAME_BASE + UDP_ULEN_OFFSET + 1] ) & 0x00FF);

  /* UDP checksum validation*/
  RetVal = uChecksum16Calc(pUserBufferPtr, uIPLen + UDP_FRAME_BASE, uIPLen + UDP_FRAME_BASE + uUDPLength - 1, &uCheckTemp, 0, uCheckTemp);
  if(RetVal){
    return DHCP_RETURN_FAIL;
  }

  if (uCheckTemp != 0xFFFF){
    xil_printf("DHCP: RX - UDP Checksum %04x - Invalid!\r\n", uCheckTemp);
    return DHCP_RETURN_INVALID;
  }
#endif

  return DHCP_RETURN_OK;   /* valid reply */
}

/********** DHCP API helper functions **********/

//=================================================================================
//  uDHCPSetGotMsgFlag
//---------------------------------------------------------------------------------
//  This method is called once a valid DHCP message is found in the user rx buffer.
//  Use in conjuction with uDHCPMessageValidate().
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetGotMsgFlag(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetStateMachineEnable
//---------------------------------------------------------------------------------
//  This method enables the DHCP state machine.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  uEnable         IN    set to SM_TRUE or SM_FALSE
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetStateMachineEnable(struct sIFObject *pIFObjectPtr, u8 uEnable){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  switch (uEnable){
    case SM_TRUE:
      vDHCPAuxSetFlag( (u8 *) &(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
      break;

    case SM_FALSE:
      vDHCPAuxClearFlag( (u8 *) &(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
      break;

    default:
      return DHCP_RETURN_FAIL;
  }

  return DHCP_RETURN_OK;
}

//=================================================================================
//  vDHCPStateMachineReset
//---------------------------------------------------------------------------------
//  This method resets the DHCP state machine (re-enable state machine after this call)
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 vDHCPStateMachineReset(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* stop the state machine */
  vDHCPAuxClearFlag((u8 *)&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
  /* reset internal state */
  pDHCPObjectPtr->tDHCPCurrentState = INIT;

  return DHCP_RETURN_OK;
}

//=================================================================================
//  vDHCPSetHostName
//---------------------------------------------------------------------------------
//  This method sets the host name to be requested in a DHCP message.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  stringHostName  IN    pointer to host name string. Limited to HOST_NAME_MAX_SIZE.
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
/* this function only copies a max of HOST_NAME_MAX_SIZE characters */
u8 vDHCPSetHostName(struct sIFObject *pIFObjectPtr, const char *stringHostName){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (stringHostName == NULL){
    return DHCP_RETURN_FAIL;
  }

  if (!strncpy((char *) pDHCPObjectPtr->arrDHCPHostName, stringHostName, HOST_NAME_MAX_SIZE)){
    return DHCP_RETURN_FAIL;
  }

  pDHCPObjectPtr->arrDHCPHostName[HOST_NAME_MAX_SIZE - 1] = '\0'; /* terminate safely */
  vDHCPAuxSetFlag( (u8 *) &(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_MSG_REQ_HOST_NAME);

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetWaitOnInitFlag
//---------------------------------------------------------------------------------
//  This method sets a flag to delay the start of dhcp by a fixed offset.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetWaitOnInitFlag(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_INIT_WAIT_EN);

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetInitWait
//---------------------------------------------------------------------------------
//  This method sets the delay timeout value at the start of dhcp.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  uInitWait       IN    count value to wait before starting dhcp
//                        time(ms) = uInitWait x POLL_INTERVAL
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetInitWait(struct sIFObject *pIFObjectPtr, u32 uInitWait){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* check that the requested init wait counter value is reasonable */
  /* let's say a reasonable waiting time is between 0 and 120s */
  /* i.e. 1200 x POLL_INTERVAL when POLL_INTERVAL = 100 (ms)*/
  /* TODO: create a macro define for the following max value */

  /* unsigned value so only have to check upper bound */
  if (uInitWait > 1200){
    debug_printf("DHCP [%02x] DHCP init wait value of %d ms out of bounds.\r\n", (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF, uInitWait * POLL_INTERVAL);
    return DHCP_RETURN_FAIL;
  }

  pDHCPObjectPtr->uDHCPSMInitWait = uInitWait;

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetRetryInterval
//---------------------------------------------------------------------------------
//  This method sets the retry interval timeout value during dhcp states.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  uRetryinterval  IN    count value to wait before retrying
//                        time(ms) = uRetryInterval x POLL_INTERVAL
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetRetryInterval(struct sIFObject *pIFObjectPtr, u32 uRetryInterval){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* check that the requested retry counter value is reasonable */
  /* let's say a reasonable retry time is between 0.5s and 30s */
  /* i.e. 5 x POLL_INTERVAL or 300 x POLL_INTERVAL when POLL_INTERVAL = 100 (ms)*/
  /* TODO: create a macro define for the following min/max values */
  if ((uRetryInterval < 5) || (uRetryInterval > 300)){
    debug_printf("DHCP [%02x] DHCP retry value of %d ms out of bounds.\r\n", (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF, uRetryInterval * POLL_INTERVAL);
    return DHCP_RETURN_FAIL;
  }

  pDHCPObjectPtr->uDHCPSMRetryInterval = uRetryInterval;

  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetRequestCachedIP
//---------------------------------------------------------------------------------
//  This method is used to pre-load the state machine with an IP to be requested
//  from the DHCP server i.e. the INIT-REBOOT state of the DHCP state machine
//  (pg 35 RFC 2131 Mar 1997). This function must be called before the state machine
//  is enabled.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  uCachedIP       IN    The IP to request from the DHCP server
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetRequestCachedIP(struct sIFObject *pIFObjectPtr, u32 uCachedIP){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  pDHCPObjectPtr->arrDHCPAddrYIPCached[0] =  ((uCachedIP >> 24) & 0xff);
  pDHCPObjectPtr->arrDHCPAddrYIPCached[1] =  ((uCachedIP >> 16) & 0xff);
  pDHCPObjectPtr->arrDHCPAddrYIPCached[2] =  ((uCachedIP >>  8) & 0xff);
  pDHCPObjectPtr->arrDHCPAddrYIPCached[3] =  ( uCachedIP        & 0xff);

  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_USE_CACHED_IP);

  debug_printf("DHCP [%02x] setting cached IP to %d.%d.%d.%d\r\n",
      pIFObjectPtr->uIFEthernetId,
      pDHCPObjectPtr->arrDHCPAddrYIPCached[0],
      pDHCPObjectPtr->arrDHCPAddrYIPCached[1],
      pDHCPObjectPtr->arrDHCPAddrYIPCached[2],
      pDHCPObjectPtr->arrDHCPAddrYIPCached[3]);

  return DHCP_RETURN_OK;
}

#if 0
//=================================================================================
//  uDHCPGetTimeoutStatus
//---------------------------------------------------------------------------------
//  
//  
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  
//=================================================================================
u8 uDHCPGetTimeoutStatus(struct sDHCPObject *pDHCPObjectPtr){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return 0;
  }

  return pDHCPObjectPtr->uDHCPTimeoutStatus; 
}
#endif

/********** DHCP API event callback registration **********/

//=================================================================================
//  eventDHCPOnMsgBuilt
//---------------------------------------------------------------------------------
//  This method registers the callback to be invoked once a complete message exists
//  in the user supplied tx buffer.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  callback        IN    function pointer of type defined form tcallUserFunction
//  pDataPtr        IN    pointer to a data object to be passed to the callback
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
int eventDHCPOnMsgBuilt(struct sIFObject *pIFObjectPtr, tcallUserFunction callback, void *pDataPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  pDHCPObjectPtr->callbackOnMsgBuilt = callback;
  pDHCPObjectPtr->pMsgDataPtr = pDataPtr;

  return DHCP_RETURN_OK;
}

//=================================================================================
//  eventDHCPOnLeaseAcqd
//---------------------------------------------------------------------------------
//  This method registers the callback to be invoked once a the DHCP lease has been
//  successfully acquired.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  callback        IN    function pointer of type defined form tcallUserFunction
//  pDataPtr        IN    pointer to a data object to be passed to the callback
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
int eventDHCPOnLeaseAcqd(struct sIFObject *pIFObjectPtr, tcallUserFunction callback, void *pDataPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  pDHCPObjectPtr->callbackOnLeaseAcqd = callback;
  pDHCPObjectPtr->pLeaseDataPtr = pDataPtr;

  return DHCP_RETURN_OK;
}

/********** DHCP API state machine functions  **********/

//=================================================================================
//  uDHCPStateMachine
//---------------------------------------------------------------------------------
//  This method invokes the DHCP state machine. The rate at which this method is
//  called is important for internal state timers (e.g. renewal timer, etc.)
//  See POLL_INTERVAL in dhcp.h file.
//
//  Parameter       Dir     Description
//  ---------       ---     -----------
//  pIFObjectPtr    IN      handle to IF state object
//
//  Return
//  ------
//  DHCP_RETURN_OK, DHCP_RETURN_FAIL or DHCP_RETURN_NOT_ENABLED
//=================================================================================
u8 uDHCPStateMachine(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN) == statusCLR){
    return DHCP_RETURN_NOT_ENABLED;
  }

  //pDHCPObjectPtr->uDHCPTimeout++;
  pDHCPObjectPtr->uDHCPCurrentClkTick++;

#if 0
  /* this logic prevents statemachine getting trapped in a particular state */
  if (pDHCPObjectPtr->tDHCPCurrentState < BOUND){
    /* after <t> seconds, if still not bound, reset state machine */
    if (pDHCPObjectPtr->uDHCPTimeout >= pDHCPObjectPtr->uDHCPSMRetryInterval){
      pDHCPObjectPtr->uDHCPTimeout = 0;   /* reset timeout counter  */
      /* pDHCPObjectPtr->tDHCPCurrentState = INIT; */         /* go to RANDOMIZE state rather
                                                                 in order to send another
                                                                 DHCP_DISCOVER immediately */
      /* pDHCPObjectPtr->uDHCPRandomWait = 0; */              /* set to zero -> don't wait in RANDOMIZE state */

      pDHCPObjectPtr->uDHCPRandomWait = pDHCPObjectPtr->uDHCPRandomWaitCached; /* wait a prerequisite amount of time before dhcp'ing */
      pDHCPObjectPtr->tDHCPCurrentState = RANDOMIZE;
      /* check if we must stop trying eventually...else keep trying */
      /* if we've retried <n> times, quit trying */
      if (pDHCPObjectPtr->uDHCPRetries >= DHCP_SM_RETRIES){
        /* pDHCPObjectPtr->uDHCPTimeoutStatus = 1; */
        if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER) == statusCLR){
          pDHCPObjectPtr->uDHCPRetries = 0;
          /* stop state machine */
          vDHCPAuxClearFlag((u8 *)&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
          /* dhcp failed, increase error counter */
          pDHCPObjectPtr->uDHCPErrors++;
          return DHCP_RETURN_FAIL;
        }
      }
    }
  }
#endif

  /* check if we must stop trying eventually...else keep trying */
  /* if we've retried <n> times, quit trying */
  if (pDHCPObjectPtr->uDHCPRetries >= DHCP_SM_RETRIES){
    /* pDHCPObjectPtr->uDHCPTimeoutStatus = 1; */
    if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER) == statusCLR){
      /* stop state machine */
      vDHCPAuxClearFlag((u8 *)&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
      /* dhcp failed, increase error counter */
      pDHCPObjectPtr->uDHCPErrors++;
      return DHCP_RETURN_FAIL;
    }
  }

  /* do the work */
  pDHCPObjectPtr->tDHCPCurrentState = dhcp_state_table[pDHCPObjectPtr->tDHCPCurrentState](pIFObjectPtr);

  return DHCP_RETURN_OK;
}

/*********** Internal state machine functions **********/

//=================================================================================
//  init_dhcp_state
//---------------------------------------------------------------------------------
//  This method initialises the state machine parameters (timers, flags, counters).
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  RANDOMIZE (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState init_dhcp_state(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /*initialize all statemachine parameters*/
  vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
  /* TODO implement API function uDHCPSetAutoRediscoverEnable */
  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER);
  /* TODO implement API function uDHCPSetShortCircuitRenew */
  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_SHORT_CIRCUIT_RENEW);
  pDHCPObjectPtr->uDHCPErrors = 0;
  pDHCPObjectPtr->uDHCPInternalTimer = 0;
  pDHCPObjectPtr->uDHCPTimeout = 0;
  pDHCPObjectPtr->uDHCPRebootReqs = 0;

#if 0
  pDHCPObjectPtr->uDHCPRandomWait = pDHCPObjectPtr->uDHCPRandomWaitCached;     /* wait a prerequisite amount of time before dhcp'ing. This is done
                                                                                  in order to spread requests when systems are started at the same time. */
#endif

  /* FIXME NOTE: removed "mac hashing randomized wait" */
  pDHCPObjectPtr->uDHCPRandomWait = 0; 

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_INIT_WAIT_EN) == statusSET){
    pDHCPObjectPtr->uDHCPRandomWait += pDHCPObjectPtr->uDHCPSMInitWait; /* add an extra fixed offset to the dhcp init waiting time */
  }

  debug_printf("DHCP [%02x] Waiting %d ms before starting DHCP at %d ms retry rate.\r\n",
      (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF,
      (pDHCPObjectPtr->uDHCPRandomWait * POLL_INTERVAL),
      (pDHCPObjectPtr->uDHCPSMRetryInterval * POLL_INTERVAL));

  return RANDOMIZE;
}

//=================================================================================
//  randomize_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements a randomized wait state before sending DHCP Discover msg.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  RANDOMIZE or SELECT (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState randomize_dhcp_state(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (pDHCPObjectPtr->uDHCPRandomWait <= 0){
    /*FIXME: check the usefulness of this counter - perhaps better to implement
     * per message-type counter rather than a general retry counter */
    pDHCPObjectPtr->uDHCPRetries++;

    if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_USE_CACHED_IP) == statusSET){
      /*
       * attempt to acquire a pre-loaded / cached lease - skip discover step
       * RFC2131 allows for this - see INIT-REBOOT state (p35, RFC2131 Mar 1997)
       */
      debug_printf("DHCP [%02x] Requesting cached lease %d.%d.%d.%d\r\n",
          (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF,
          pDHCPObjectPtr->arrDHCPAddrYIPCached[0],
          pDHCPObjectPtr->arrDHCPAddrYIPCached[1],
          pDHCPObjectPtr->arrDHCPAddrYIPCached[2],
          pDHCPObjectPtr->arrDHCPAddrYIPCached[3]);

      if (uDHCPBuildMessage(pIFObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_DHCP_REQIP | 1<<flagDHCP_MSG_DHCP_REQPARAM
              | 1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
        pDHCPObjectPtr->uDHCPErrors++;
      } else {
        pDHCPObjectPtr->uDHCPTx++;
      }

      if (pDHCPObjectPtr->uDHCPRebootReqs >= (DHCP_REBOOTING_REQS_MAX - 1)){
        /* stop trying to request cached IP - will send DISCOVER on next cycle */
        vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_USE_CACHED_IP);
#if 0
        pDHCPObjectPtr->uDHCPRebootReqCount = 0; /* TODO: do we really want to
                                                  * clear this counter here.
                                                  * Keep around for debugging. */
#endif
      }

      pDHCPObjectPtr->uDHCPRebootReqs++;
      pDHCPObjectPtr->uDHCPTimeout = 0;       /* reset timeout counter  */
      return REQUEST;   /* skip DHCPDiscover message */
    } else {
      if (uDHCPBuildMessage(pIFObjectPtr, DHCPDISCOVER, (1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_RESET_SEC | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
        /* on failure */
        pDHCPObjectPtr->uDHCPErrors++;
      } else {
        /* on success */
        pDHCPObjectPtr->uDHCPTx++;
      }
      /* progress to the next state even on failure in order for timeout to catch the error */
      /* this prevents us getting stuck in this state */
      pDHCPObjectPtr->uDHCPTimeout = 0;       /* reset timeout counter  */
      return SELECT;
    }
  }

  pDHCPObjectPtr->uDHCPRandomWait--;
  return RANDOMIZE;
}

//=================================================================================
//  select_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the selecting state which waits for a valid DHCP Offer msg.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  SELECT or WAIT (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState select_dhcp_state(struct sIFObject *pIFObjectPtr){
  typeDHCPMessage tDHCPMsgType;
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

    tDHCPMsgType = tDHCPProcessMsg(pIFObjectPtr);
    /* expecting a DHCP OFFER message */
    if (tDHCPMsgType == DHCPOFFER){
      pDHCPObjectPtr->uDHCPRx++;
      /* NOTE: interval selections important here */
      //FIXME pDHCPObjectPtr->uDHCPRandomWait = (u32) (uDHCPRandomNumber(0) % 15 + 5);
      pDHCPObjectPtr->uDHCPRandomWait = 0;
      return WAIT;
    } else {
      pDHCPObjectPtr->uDHCPErrors++;
    }
  }

  /* Give up "selecting" after a N seconds, where N is user selected.
     Go back to the previous state in order to resend DISCOVER packet. */
  if (pDHCPObjectPtr->uDHCPTimeout >= pDHCPObjectPtr->uDHCPSMRetryInterval){
    pDHCPObjectPtr->uDHCPRandomWait = 0;    /* set to zero -> don't wait in RANDOMIZE state, send DISCOVER immediately */
    return RANDOMIZE;
  }

  pDHCPObjectPtr->uDHCPTimeout++;
  return SELECT;
}

//=================================================================================
//  wait_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements a randomized wait state before sending a DHCP Request msg.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  WAIT or REQUEST (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState wait_dhcp_state(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);
  if (pDHCPObjectPtr->uDHCPRandomWait <= 0){
    if (uDHCPBuildMessage(pIFObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_DHCP_REQIP | 1<<flagDHCP_MSG_DHCP_SVRID | 1<<flagDHCP_MSG_DHCP_REQPARAM | 1<<flagDHCP_MSG_DHCP_VENDID))){
      /* on failure */
      pDHCPObjectPtr->uDHCPErrors++;
    } else {
      pDHCPObjectPtr->uDHCPTx++;
    }
    /* progress to the next state even on failure in order for timeout to catch the error */
    /* this prevents us getting stuck in this state */
    pDHCPObjectPtr->uDHCPTimeout = 0;       /* reset timeout counter  */
    return REQUEST;
  }

  pDHCPObjectPtr->uDHCPRandomWait--;
  return WAIT;
}

#if 0
//=================================================================================
//  reboot_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the rebooting state of RFC 2131. It attempts to
//  re-acquire the last lease received after a reboot / reconfigure.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//   (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState reboot_dhcp_state(struct sIFObject *pIFObjectPtr){
}
#endif /* REBOOTING STATE:
        * this state would have the same logic as REQUEST state - TODO: should
        * we separate it though for better flexibility when implementing
        * changes? For now, just go to REQUEST state
        */

//=================================================================================
//  request_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the requesting state which waits for a valid DHCP Ack msg.
//  It sets the various lease parameters and calls the user specified callback once
//  the lease has been obtained.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  REQUEST or BOUND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState request_dhcp_state(struct sIFObject *pIFObjectPtr){
  typeDHCPMessage tDHCPMsgType;
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

    tDHCPMsgType = tDHCPProcessMsg(pIFObjectPtr);
    if (tDHCPMsgType == DHCPACK){
      pDHCPObjectPtr->uDHCPRx++;

      if (pDHCPObjectPtr->uDHCPT1 == 0){
        /* pDHCPObjectPtr->uDHCPT1 = ((pDHCPObjectPtr->uDHCPLeaseTime + 1) / 2); */
        pDHCPObjectPtr->uDHCPT1 = ((pDHCPObjectPtr->uDHCPLeaseTime) / 2);   /* NOTE: ignore truncation toward zero (to handle infinite lease case) */
      }
      if (pDHCPObjectPtr->uDHCPT2 == 0){
        pDHCPObjectPtr->uDHCPT2 = ((pDHCPObjectPtr->uDHCPLeaseTime * 7) / 8);
      }
      vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_LEASE_OBTAINED);

      /* invoke a callback once the lease is acquired */
      if (pDHCPObjectPtr->callbackOnLeaseAcqd != NULL){
        (*(pDHCPObjectPtr->callbackOnLeaseAcqd))(pIFObjectPtr, pDHCPObjectPtr->pLeaseDataPtr);
      }

      return BOUND;
    } else {
      pDHCPObjectPtr->uDHCPErrors++;
    } 
  }

  /* Give up "requesting" after a N seconds, where N is user selected.
     Go back to the previous state in order to resend DISCOVER packet. */
  if (pDHCPObjectPtr->uDHCPTimeout >= pDHCPObjectPtr->uDHCPSMRetryInterval){
    pDHCPObjectPtr->uDHCPRandomWait = 0;    /* set to zero -> don't wait in RANDOMIZE state, send DISCOVER immediately */
    return RANDOMIZE;
  }

  pDHCPObjectPtr->uDHCPTimeout++;
  return REQUEST;
}

//=================================================================================
//  bound_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the bound state which waits the prerequisite amount of 
//  time before attempting to renew the lease.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  BOUND or RENEW (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState bound_dhcp_state(struct sIFObject *pIFObjectPtr){
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if ((pDHCPObjectPtr->uDHCPInternalTimer * POLL_INTERVAL) >= (pDHCPObjectPtr->uDHCPT1 * 1000)){
    /* time to renew our lease */
    if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_SHORT_CIRCUIT_RENEW) == statusCLR){
      /* if the renew step is not short-circuited (see note in dhcp.h), send the renew DHCPREQUEST */
      if (uDHCPBuildMessage(pIFObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_UNICAST | 1<<flagDHCP_MSG_BOOTP_CIPADDR | 1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_RESET_SEC | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
        pDHCPObjectPtr->uDHCPErrors++;
      } else {
        pDHCPObjectPtr->uDHCPTx++;
      }
      return RENEW;
    } else {
      /* re-discover the dhcp lease */
      /* reset flags and counters */
      vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
      pDHCPObjectPtr->uDHCPInternalTimer = 0;
      pDHCPObjectPtr->uDHCPTimeout = 0;   /* reset timeout counter  */
      pDHCPObjectPtr->uDHCPRandomWait = 0;
      pDHCPObjectPtr->uDHCPRetries = 0;
      return RANDOMIZE;
    }
  }

  return BOUND;
}

//=================================================================================
//  renew_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the renew state which waits for the rewewal acknowledge
//  from the DHCP server, in which case it returns to the bound state. Otherwise,
//  it waits the prerequisite amount of time before attempting to rebind the lease.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  BOUND or RENEW or REBIND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState renew_dhcp_state(struct sIFObject *pIFObjectPtr){
  typeDHCPMessage tDHCPMsgType;
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

    tDHCPMsgType = tDHCPProcessMsg(pIFObjectPtr);
    /* expecting a DHCP ACK message */
    if (tDHCPMsgType == DHCPACK){
      pDHCPObjectPtr->uDHCPInternalTimer = 0;
      /* reset timer and stay bound */
      return BOUND;
    } else {
      pDHCPObjectPtr->uDHCPErrors++;
    } 
  }

  if ((pDHCPObjectPtr->uDHCPInternalTimer * POLL_INTERVAL) >= (pDHCPObjectPtr->uDHCPT2 * 1000)){
    /*  time to rebind */
    if (uDHCPBuildMessage(pIFObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_BOOTP_CIPADDR | 1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
      pDHCPObjectPtr->uDHCPErrors++;
    } else {
      pDHCPObjectPtr->uDHCPTx++;
    }
    return REBIND;
  }

  return RENEW;
}

//=================================================================================
//  rebind_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the rebind state which waits for the rebinding acknowledge
//  from the DHCP server, in which case it returns to the bound state. Otherwise,
//  it waits the prerequisite amount of time before "timing out". It tries to
//  "re-discover" the DHCP server if the relevant flag is set.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  BOUND or RANDOMIZE or REBIND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState rebind_dhcp_state(struct sIFObject *pIFObjectPtr){
  typeDHCPMessage tDHCPMsgType;
  struct sDHCPObject *pDHCPObjectPtr;

  /* cannot currently do SANE_DHCP sanity checks here since this function returns next state */
  /* could possibly return an error state and handle it */

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
    tDHCPMsgType = tDHCPProcessMsg(pIFObjectPtr);
    if (tDHCPMsgType == DHCPACK){
      pDHCPObjectPtr->uDHCPRx++;
      /* reset the timer and stay bound */
      pDHCPObjectPtr->uDHCPInternalTimer = 0;
      return BOUND;
    } else {
      pDHCPObjectPtr->uDHCPErrors++;
    } 
  }

  if ((pDHCPObjectPtr->uDHCPInternalTimer * POLL_INTERVAL) >= (pDHCPObjectPtr->uDHCPLeaseTime * 1000)){
    /* lease has expired - should we "re-discover" a DHCP lease */
    if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER) == statusCLR){
      /* stop the state machine */
      vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
    } else {
      /* reset flags and counters */
      vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
      pDHCPObjectPtr->uDHCPInternalTimer = 0;
      pDHCPObjectPtr->uDHCPRandomWait = 0;
      pDHCPObjectPtr->uDHCPRetries = 0;
    }
    return RANDOMIZE;
  }

  return REBIND; 
}

/*********** DHCP internal logic functions **********/

//=================================================================================
//  uDHCPBuildMessage
//---------------------------------------------------------------------------------
//  This method builds the relevant DHCP Message depending on the flags set and
//  places it in the user supplied tx buffer. It calls the user defined callback
//  once the message has been built.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//  tDHCPMsgType    IN    DHCP message type (ennumerated type)
//  tDHCPMsgFlags   IN    flags to set various message parameters (enumerated type)
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
static u8 uDHCPBuildMessage(struct sIFObject *pIFObjectPtr, typeDHCPMessage tDHCPMsgType, typeDHCPMessageFlags tDHCPMsgFlags){
  /* TODO: separate out the udp,ip,eth layers and then use encapusalted layer style to build packets */
  u8 *pBuffer;
  u8  uPaddingLength = 0;
  u16 uSize;
  u16 uIndex;
  u16 uCachedIndex;
  u16 uTmpIndex;
  u16 uLength;
  u16 uChecksum;
  u32 uSeconds = 0;
  u32 uTempXid = 0;
  u8 uPseudoHdr[12] = {0};
  int RetVal = (-1);
  u16 uCheckTemp = 0;
  u16 uUDPLength = 0;

  struct sDHCPObject *pDHCPObjectPtr;

  SANE_DHCP(pIFObjectPtr);

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  /* simplify ur life */
  pBuffer = pIFObjectPtr->pUserTxBufferPtr;

  uSize = pIFObjectPtr->uUserTxBufferSize;

  /* zero the buffer, saves us from having to explicitly set zero valued bytes */
  memset(pBuffer, 0, uSize);

  /*****  ethernet frame stuff  *****/ 

  /* if unicast, fill in cached next hop mac address, else broadcast */
  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_UNICAST) == statusSET){
    memcpy(pBuffer + ETH_DST_OFFSET, pDHCPObjectPtr->arrDHCPNextHopMacCached, 6);
  } else {
    memset(pBuffer + ETH_DST_OFFSET, 0xff, 6);   /* broadcast */
  } 

  /* fill in our hardware mac address */
  memcpy(pBuffer + ETH_SRC_OFFSET, pIFObjectPtr->arrIFAddrMac, 6);

  /* ethernet frame type */
  pBuffer[ETH_FRAME_TYPE_OFFSET] = 0x08;

  /*****  ip frame struff  *****/
  pBuffer[IP_FRAME_BASE + IP_FLAG_FRAG_OFFSET] = 0x40;    /* ip flags = don't fragment */
  pBuffer[IP_FRAME_BASE + IP_V_HIL_OFFSET] = 0x45;

  pBuffer[IP_FRAME_BASE + IP_TTL_OFFSET] = 0x80;
  pBuffer[IP_FRAME_BASE + IP_PROT_OFFSET] = 0x11;

  /* if unicast, fill in relevant addresses else broadcast */
  if ((tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_UNICAST) == statusSET) && \
      ((pDHCPObjectPtr->arrDHCPAddrServerCached[0] != '\0') && (pDHCPObjectPtr->arrDHCPAddrYIPCached[0] != '\0'))){
    memcpy(pBuffer + IP_FRAME_BASE + IP_DST_OFFSET, pDHCPObjectPtr->arrDHCPAddrServerCached, 4);
    memcpy(pBuffer + IP_FRAME_BASE + IP_SRC_OFFSET, pDHCPObjectPtr->arrDHCPAddrYIPCached, 4);
  } else {
    memset(pBuffer + IP_FRAME_BASE + IP_DST_OFFSET, 0xff, 4);     /* broadcast */
    pBuffer[BOOTP_FRAME_BASE + BOOTP_FLAGS_OFFSET] = 0x80;        /* set the broadcast bit flag in the bootp/dhcp payload */
  }

  /*****  udp frame struff  *****/
  pBuffer[UDP_FRAME_BASE + UDP_SRC_PORT_OFFSET + 1] = 0x44;
  pBuffer[UDP_FRAME_BASE + UDP_DST_PORT_OFFSET + 1] = 0x43;

  /*****  bootp stuff *****/
  pBuffer[BOOTP_FRAME_BASE + BOOTP_OPTYPE_OFFSET] = 0x01;
  pBuffer[BOOTP_FRAME_BASE + BOOTP_HWTYPE_OFFSET] = 0x01;
  pBuffer[BOOTP_FRAME_BASE + BOOTP_HWLEN_OFFSET] = 0x06;

  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_NEW_XID) == statusSET){
    uTempXid = ((pIFObjectPtr->arrIFAddrMac[2] & 0x0f) << 28) + \
               ((pIFObjectPtr->arrIFAddrMac[3] & 0x0f) << 24) + \
               ((pIFObjectPtr->arrIFAddrMac[4] & 0x0f) << 20) + \
               ((pIFObjectPtr->arrIFAddrMac[5] & 0x0f) << 16);
    uTempXid = uTempXid & 0xffff0000;
    pDHCPObjectPtr->uDHCPXidCached = uDHCPRandomNumber(uTempXid); /* seed with serial number and i/f id */
  }

  pBuffer[BOOTP_FRAME_BASE + BOOTP_XID_OFFSET    ] =  ((pDHCPObjectPtr->uDHCPXidCached >> 24) & 0xff);
  pBuffer[BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 1] =  ((pDHCPObjectPtr->uDHCPXidCached >> 16) & 0xff);
  pBuffer[BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 2] =  ((pDHCPObjectPtr->uDHCPXidCached >>  8) & 0xff);
  pBuffer[BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 3] =  ((pDHCPObjectPtr->uDHCPXidCached      ) & 0xff);


  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_RESET_SEC) != statusSET){
    uSeconds = (pDHCPObjectPtr->uDHCPCurrentClkTick - pDHCPObjectPtr->uDHCPCachedClkTick) * pDHCPObjectPtr->uDHCPExternalTimerTick; 

    pBuffer[BOOTP_FRAME_BASE + BOOTP_SEC_OFFSET    ] = (u8) ((uSeconds & 0xFF00) >> 8);
    pBuffer[BOOTP_FRAME_BASE + BOOTP_SEC_OFFSET + 1] = (u8) ( uSeconds & 0xFF);
  } else {
    /* Store current clk tick */
    pDHCPObjectPtr->uDHCPCachedClkTick = pDHCPObjectPtr->uDHCPCurrentClkTick;
    /* seconds bytes in buffer will be zero due to earlier memset */
  }

  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_BOOTP_CIPADDR) == statusSET){
    memcpy(pBuffer + BOOTP_FRAME_BASE + BOOTP_CIPADDR_OFFSET, pDHCPObjectPtr->arrDHCPAddrYIPCached, 4);
  }

  memcpy(pBuffer + BOOTP_FRAME_BASE + BOOTP_CHWADDR_OFFSET, pIFObjectPtr->arrIFAddrMac, 6);

  /*****  end of the fixed part of the message, now on to the variable length DHCP options part, extension to bootp  *****/

  /* TODO: FIXME here we need to constantly check for buffer overruns */

  uIndex = 0;

  /* obfuscation alert: 'index++' - index gets incremented after data gets stored at array index */

  /* DHCP magic cookie - mandatory */
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 0x63;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 0x82;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 0x53;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 0x63;

  /* DHCP message type option - mandatory */
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 53;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 1;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = (u8) tDHCPMsgType;

  /* DHCP client-identifier option - always enabled */
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 61;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 7;
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 1;
  memcpy(pBuffer + DHCP_OPTIONS_BASE + uIndex, pIFObjectPtr->arrIFAddrMac, 6);
  uIndex += 6;

  /* DHCP hostname support - enabled by API function */
  if (tDHCPAuxTestFlag( (u8 *) &(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_MSG_REQ_HOST_NAME) == statusSET){
    if (pDHCPObjectPtr->arrDHCPHostName[0] != '\0'){
      pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 12;
      uCachedIndex = uIndex++;  /* cache the index where length should be stored, then increment index */

      for ( uTmpIndex = 0; ((uTmpIndex < HOST_NAME_MAX_SIZE - 1) && (pDHCPObjectPtr->arrDHCPHostName[uTmpIndex] != '\0')) ; uTmpIndex++){
        pBuffer[DHCP_OPTIONS_BASE + uIndex++] = pDHCPObjectPtr->arrDHCPHostName[uTmpIndex];
      }

      pBuffer[DHCP_OPTIONS_BASE + uCachedIndex] = uTmpIndex;
    }
  }

  /* DHCP requested IP address - enabled by flag */
  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_REQIP) == statusSET){
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 50;
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 4;
    memcpy(pBuffer + DHCP_OPTIONS_BASE + uIndex, pDHCPObjectPtr->arrDHCPAddrYIPCached, 4);
    uIndex += 4;
  }

  /* DHCP server identifier - enabled by flag */
  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_SVRID) == statusSET){
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 54;
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 4;
    memcpy(pBuffer + DHCP_OPTIONS_BASE + uIndex, pDHCPObjectPtr->arrDHCPAddrServerCached, 4);
    uIndex += 4;
  }

  /* DHCP parameter request list - enabled by flag */
  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_REQPARAM) == statusSET){
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 55;
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 5;
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 1;  //subnet mask
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 3;  //router
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 6;  //domain name server
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 12; //host name
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 15; //domain name
  }

#pragma message "vendor id length is " STRING(VENDOR_ID_LEN)

  /* DHCP vendor class identifier - enabled by flag */
  if (tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_DHCP_VENDID) == statusSET){
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 60;
    pBuffer[DHCP_OPTIONS_BASE + uIndex++] = VENDOR_ID_LEN;
    memcpy(pBuffer + DHCP_OPTIONS_BASE + uIndex, VENDOR_ID, VENDOR_ID_LEN);
    uIndex += VENDOR_ID_LEN;
  }

  /* DHCP end option - indicates end of DHCP message options */
  pBuffer[DHCP_OPTIONS_BASE + uIndex++] = 0xff;

  /* calculate and fill in udp frame packet lengths */
  uLength = UDP_HEADER_TOTAL_LEN + BOOTP_FRAME_TOTAL_LEN + uIndex;
  uUDPLength = uLength; /* save for pseudo header later */
  pBuffer[UDP_FRAME_BASE + UDP_ULEN_OFFSET    ] = (u8) ((uLength & 0xff00) >> 8);
  pBuffer[UDP_FRAME_BASE + UDP_ULEN_OFFSET + 1] = (u8) (uLength & 0xff);

  /* calculate and fill in ip frame packet lengths */
  uLength = uLength + IP_FRAME_TOTAL_LEN;
  pBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET    ] = (u8) ((uLength & 0xff00) >> 8);
  pBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET + 1] = (u8) (uLength & 0xff);

  /* calculate checksums - ip mandatory, udp optional */
  uChecksum16Calc(pBuffer, IP_FRAME_BASE, UDP_FRAME_BASE - 1, &uChecksum, 0, 0);
  pBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET    ] = (u8) ((uChecksum & 0xff00) >> 8);
  pBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET + 1] = (u8) (uChecksum & 0xff);

  /* calculate udp checksum value */

  /* build the pseudo IP header for UDP checksum calculation */
  memcpy(&(uPseudoHdr[0]), pBuffer + IP_FRAME_BASE + IP_SRC_OFFSET, 4);
  memcpy(&(uPseudoHdr[4]), pBuffer + IP_FRAME_BASE + IP_DST_OFFSET, 4);
  uPseudoHdr[8] = 0;
  uPseudoHdr[9] = pBuffer[IP_FRAME_BASE + IP_PROT_OFFSET];
  memcpy(&(uPseudoHdr[10]), pBuffer + UDP_FRAME_BASE + UDP_ULEN_OFFSET, 2);

  trace_printf("DHCP: UDP pseudo header: \r\n");
  for (int i = 0; i < 12; i++){
    trace_printf(" %02x", uPseudoHdr[i]);
  }
  trace_printf("\r\n");

  RetVal = uChecksum16Calc(&(uPseudoHdr[0]), 0, 11, &uCheckTemp, 0, 0);
  if(RetVal){
    return DHCP_RETURN_FAIL;
  }

  /* UDP checksum validation*/
  RetVal = uChecksum16Calc(pBuffer, UDP_FRAME_BASE, UDP_FRAME_BASE + uUDPLength - 1, &uCheckTemp, 0, uCheckTemp);
  if(RetVal){
    return DHCP_RETURN_FAIL;
  }

  trace_printf("DHCP: UDP checksum value = %04x\r\n", uCheckTemp);

  pBuffer[UDP_FRAME_BASE + UDP_CHKSM_OFFSET    ] = (u8) ((uCheckTemp & 0xff00) >> 8);
  pBuffer[UDP_FRAME_BASE + UDP_CHKSM_OFFSET + 1] = (u8) (uCheckTemp & 0xff);

  /* pad to nearest 64 bit boundary */
  /* simply increase total length by following amount - these bytes already zero due to earlier memset */
  uPaddingLength = 8 - ((uLength + ETH_FRAME_TOTAL_LEN) % 8);

  /* and with 0b111 since only interested in values of 0 to 7 */
  uPaddingLength = uPaddingLength & 0x7; 

  pIFObjectPtr->uMsgSize = uLength + ETH_FRAME_TOTAL_LEN + uPaddingLength;

  /* TODO: set a message ready flag */

  debug_printf("DHCP [%02x] sending DHCP %s with xid 0x%x\r\n",
      (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF,
      dhcp_msg_string_lookup[tDHCPMsgType], pDHCPObjectPtr->uDHCPXidCached);

  /* invoke a callback once the message successfully built */
  if (pDHCPObjectPtr->callbackOnMsgBuilt != NULL){
    (*(pDHCPObjectPtr->callbackOnMsgBuilt))(pIFObjectPtr, pDHCPObjectPtr->pMsgDataPtr);
  }

  return DHCP_RETURN_OK;
}

//=================================================================================
//  tDHCPProcessMsg
//---------------------------------------------------------------------------------
//  This method processes a valid DHCP message located in the user supplied rx buffer
//  and extracts all the necessary lease parameters.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  typeDHCPMessage (enumerated type)
//=================================================================================
static typeDHCPMessage tDHCPProcessMsg(struct sIFObject *pIFObjectPtr){
  u16 uOptionIndex = 0;
  u16 uOptionEnd = 0;
  u8 uOption = 0;
  u8 uTmpIndex = 0;

  u8 uIPLen = 0;
  u8 *pBuffer;
  u32 uDHCPTmpXid = 0;
  typeDHCPMessage tDHCPMsgType = DHCPERROR;

  struct sDHCPObject *pDHCPObjectPtr;

  /* guard separately - can't call SANE_DHCP() since return value to upper layer */
  /* API is different (message type). TODO: Could possibly return usual value (DHCP_RETURN_*) and write */
  /* message type to an address passed into the function as an argument. */
#ifdef DO_SANITY_CHECKS
  if (pIFObjectPtr == NULL){
    return DHCPERROR;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    return DHCPERROR;
  }
#endif

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

#if 0
#ifdef DO_SANITY_CHECKS
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCPERROR;
  }
#endif
#endif

#ifdef DO_SANITY_CHECKS
  /* check if the object has been initialised */
  /* has uDHCPInit been called to initialize dependancies? */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCPERROR;
  }
#endif

  /* simplify your life */
  pBuffer = pIFObjectPtr->pUserRxBufferPtr;

#ifdef DO_SANITY_CHECKS
  if (pBuffer == NULL){
    return DHCPERROR;
  }
#endif

  /* adjust ip base value if ip length greater than 20 bytes */
  uIPLen = (((pBuffer[IP_FRAME_BASE] & 0x0F) * 4) - 20);

  uDHCPTmpXid = uDHCPTmpXid | (u32) pBuffer[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET    ] << 24;
  uDHCPTmpXid = uDHCPTmpXid | (u32) pBuffer[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 1] << 16;
  uDHCPTmpXid = uDHCPTmpXid | (u32) pBuffer[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 2] <<  8;
  uDHCPTmpXid = uDHCPTmpXid | (u32) pBuffer[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + 3];

  /* compared xid with last cached xid, just in case there's a duplicate packet lingering in some buffer somewhere*/
  if (pDHCPObjectPtr->uDHCPXidCached != uDHCPTmpXid){
    pDHCPObjectPtr->uDHCPInvalid++;
    return DHCPERROR;
  }

  /* get mac address of the "next-hop" server/router which sent us this packet */
  memcpy(pDHCPObjectPtr->arrDHCPNextHopMacCached, pBuffer + ETH_SRC_OFFSET, 6);

  /* get the offered ip addr */
  memcpy(pDHCPObjectPtr->arrDHCPAddrYIPCached, pBuffer + uIPLen + BOOTP_FRAME_BASE + BOOTP_YIPADDR_OFFSET, 4);

  /* add 4 to jump past dhcp magic cookie in data buffer */
  uOptionIndex = uIPLen + DHCP_OPTIONS_BASE + 4;

  /* TODO: FIXME here we need to constantly check for buffer overruns */

  while(uOptionEnd == 0){
    uOption = pBuffer[uOptionIndex];

    switch(uOption){
      case 53:        /* message type */
        tDHCPMsgType = (typeDHCPMessage) pBuffer[uOptionIndex + 2];
        uOptionIndex = uOptionIndex + 3; 
        break;

      case 1:         /* subnet mask */
        memcpy(pDHCPObjectPtr->arrDHCPAddrSubnetMask, pBuffer + uOptionIndex + 2, 4);
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

      case 3:         /* router */
        memcpy(pDHCPObjectPtr->arrDHCPAddrRoute, pBuffer + uOptionIndex + 2, 4);
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

      case 51:        //lease time
        for (uTmpIndex = 0; uTmpIndex < 4; uTmpIndex++){
          pDHCPObjectPtr->uDHCPLeaseTime = (pDHCPObjectPtr->uDHCPLeaseTime << 8) + pBuffer[uOptionIndex + 2 + uTmpIndex];
        }
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

      case 58:        //Renewal (T1) Time Value
        for (uTmpIndex = 0; uTmpIndex < 4; uTmpIndex++){
          pDHCPObjectPtr->uDHCPT1 = (pDHCPObjectPtr->uDHCPT1 << 8) + pBuffer[uOptionIndex + 2 + uTmpIndex];
        }
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

      case 59:        //Rebinding (T2) Time Value
        for (uTmpIndex = 0; uTmpIndex < 4; uTmpIndex++){
          pDHCPObjectPtr->uDHCPT2 = (pDHCPObjectPtr->uDHCPT2 << 8) + pBuffer[uOptionIndex + 2 + uTmpIndex];
        }
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

        /* get the server addr - retrieved from the Server ID dhcp option included in DHCPOFFER (see rfc 2132, par 9.7) */
      case 54:        //Server ID
        memcpy(pDHCPObjectPtr->arrDHCPAddrServerCached, pBuffer + uOptionIndex + 2, 4);
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;

      case 255:       //end option
        uOptionEnd = 1;
        break;

      default:
        uOptionIndex = uOptionIndex + pBuffer[uOptionIndex + 1] + 2;
        break;
    }
  }

  debug_printf("DHCP [%02x] processed DHCP %s with xid 0x%x\r\n",
      (pIFObjectPtr != NULL) ? pIFObjectPtr->uIFEthernetId : 0xFF,
      dhcp_msg_string_lookup[tDHCPMsgType], uDHCPTmpXid);

  return tDHCPMsgType;
}

/**********  Some auxiliary functions **********/

//=================================================================================
//  uDHCPWait
//---------------------------------------------------------------------------------
//  This method "hashes" the MAC address to generate a waiting time between 1 and 30 counts.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  The number of counts to wait
//=================================================================================
static u8 uDHCPWait(struct sIFObject *pIFObjectPtr){
  u16 uWaitCount = 0;
  u8 uIndex;

  /** very simple division hashing **/

  /* sum the octets of the mac address */
  for (uIndex = 0; uIndex < 6; uIndex++){
    uWaitCount += pIFObjectPtr->arrIFAddrMac[uIndex];
  }

  /* add the 5th octet to sum and multiply by sum */
  /* 5th octet chosen since it changes most frequently across SKARABs */
  uWaitCount = uWaitCount * (uWaitCount + pIFObjectPtr->arrIFAddrMac[4]);

  /* modulo operation by largest prime under 30 */
  uWaitCount = (uWaitCount % 29) + 1;

  return uWaitCount;
}

//=================================================================================
//  uDHCPRandomNumber
//---------------------------------------------------------------------------------
//  This method generates a pseudo "random" number.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  uSeed           IN    Seed the random number generator with this value.
//
//  Return
//  ------
//  A pseudo "random" number
//=================================================================================
/* TODO: just an upcount of the seed value for now */
static u32 uDHCPRandomNumber(u32 uSeed){
  static u32 uPRN = 0;

  if (uPRN){
    uPRN++;
  } else {
    uPRN = uSeed;
  }

  return uPRN;
}

//=================================================================================
//  vDHCPAuxClearBitFlag
//---------------------------------------------------------------------------------
//  This auxiliary method clears a bit in the flag register.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pFlagRegister   IN    pointer to the register holding the flag to be cleared
//  tBitPosition    IN    bit/flag to be cleared. enumerated typeDHCPRegisterFlags 
//
//  Return
//  ------
//  None
//=================================================================================
static void vDHCPAuxClearFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition){
  u8 uMask;

  if (pFlagRegister == NULL){
    return;
  }

  if (tBitPosition > 7){
    return;
  }

  uMask = ~(1<<tBitPosition);
  (*pFlagRegister) = (*pFlagRegister) & uMask; 
}

//=================================================================================
//  vDHCPAuxSetFlag
//---------------------------------------------------------------------------------
//  This auxiliary method sets a bit in the flag register.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pFlagRegister   IN    pointer to the register holding the flag to be set
//  tBitPosition    IN    bit/flag to be set. enumerated typeDHCPRegisterFlags 
//
//  Return
//  ------
//  None
//=================================================================================
static void vDHCPAuxSetFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition){
  u8 uMask;

  if (pFlagRegister == NULL){
    return;
  }

  if (tBitPosition > 7){
    return;
  }

  uMask = 1<<tBitPosition;
  (*pFlagRegister) = (*pFlagRegister) | uMask;
}

//=================================================================================
//  tDHCPAuxTestBitFlag
//---------------------------------------------------------------------------------
//  This auxiliary method tests a bit in the flag register.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pFlagRegister   IN    pointer to the register holding the flag to be tested
//  tBitPosition    IN    bit/flag to test. enumerated typeDHCPRegisterFlags 
//
//  Return
//  ------
//  statusSET or statusCLR of enumerated type typeDHCPFlagStatus
//=================================================================================
static typeDHCPFlagStatus tDHCPAuxTestFlag(u8 *pFlagRegister, typeDHCPRegisterFlags tBitPosition){
  u8 uMask;
  uMask = 1<<tBitPosition;

  if (pFlagRegister == NULL){
    return statusERR;
  }

  if (tBitPosition > 7){
    return statusERR;
  }

  return ( ((*pFlagRegister) & uMask) ? statusSET : statusCLR);
}
