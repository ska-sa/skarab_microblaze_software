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
#include <stdio.h>
#include <string.h>

/* local includes */
#include "dhcp.h"

/* Local function prototypes  */
static typeDHCPState init_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState randomize_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState select_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState wait_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState request_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState bound_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState renew_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);
static typeDHCPState rebind_dhcp_state(struct sDHCPObject *pDHCPObjectPtr);

static u8 uDHCPBuildMessage(struct sDHCPObject *pDHCPObjectPtr, typeDHCPMessage tDHCPMsgType, typeDHCPMessageFlags tDHCPMsgFlags);
static typeDHCPMessage tDHCPProcessMsg(struct sDHCPObject *pDHCPObjectPtr);

static u16 uChecksum16Calc(u8 *pDataPtr, u16 uIndexStart, u16 uIndexEnd);
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


/*********** DHCP API functions **********/

//=================================================================================
//  uDHCPInit
//---------------------------------------------------------------------------------
//  Call this method to initialize the DHCP state object. This method must be called
//  first before invoking the DHCP state machine.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  pRxBufferPtr    IN    ptr to user supplied rx buffer which will contain rx dhcp packet
//  uRxBufferSize   IN    size of the user specified rx buffer
//  pTxBufferPtr    OUT   ptr to user supplied tx buffer where dhcp message will be "built up"
//  uRxBufferSize   IN    size of the user specified tx buffer
//  arrUserMacAddr  IN    ptr to byte array containing the 6 octets of the hardware mac address
//
//  Return
//  ------
// DHCP_RETURN_OK or DHCP_RETURN_FAIL 
//=================================================================================
u8 uDHCPInit(struct sDHCPObject *pDHCPObjectPtr, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize, u8 *arrUserMacAddr){
  u8 uLoopIndex;

  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  if (pRxBufferPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  if (pTxBufferPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  if ( arrUserMacAddr == NULL){
    return DHCP_RETURN_FAIL;
  }

  /* check that user supplied a reasonable amount of buffer space */
  if (uRxBufferSize < DHCP_MIN_BUFFER_SIZE){
    return DHCP_RETURN_FAIL;
  }

  if (uTxBufferSize < DHCP_MIN_BUFFER_SIZE){
    return DHCP_RETURN_FAIL;
  }

  pDHCPObjectPtr->pUserRxBufferPtr = pRxBufferPtr;
  pDHCPObjectPtr->uUserRxBufferSize = uRxBufferSize;

  pDHCPObjectPtr->pUserTxBufferPtr = pRxBufferPtr;
  pDHCPObjectPtr->uUserTxBufferSize = uTxBufferSize;

  memset(pTxBufferPtr, 0x0, (size_t) uTxBufferSize);

  pDHCPObjectPtr->uDHCPMsgSize = 0;

  pDHCPObjectPtr->tDHCPCurrentState = INIT;
  
  pDHCPObjectPtr->uDHCPMessageReady = 0;
  pDHCPObjectPtr->uDHCPTx = 0;
  pDHCPObjectPtr->uDHCPRx = 0;
  pDHCPObjectPtr->uDHCPErrors = 0;
  pDHCPObjectPtr->uDHCPInvalid = 0;
  pDHCPObjectPtr->uDHCPRetries = 0;

  pDHCPObjectPtr->uDHCPTimeout = 0;

  pDHCPObjectPtr->uDHCPInternalTimer = 0;
  pDHCPObjectPtr->uDHCPCurrentClkTick = 0;
  pDHCPObjectPtr->uDHCPCachedClkTick = 0;
  pDHCPObjectPtr->uDHCPExternalTimerTick = 0;
  pDHCPObjectPtr->uDHCPRandomWait = 0;

  pDHCPObjectPtr->uDHCPT1 = 0;
  pDHCPObjectPtr->uDHCPT2 = 0;
  pDHCPObjectPtr->uDHCPLeaseTime = 0;

  for (uLoopIndex = 0; uLoopIndex < 6; uLoopIndex++){
    pDHCPObjectPtr->arrDHCPNextHopMacCached[uLoopIndex] = 0xff;
    /* FIXME: possible buffer overrun if user issues smaller array - rather use string logic */
    pDHCPObjectPtr->arrDHCPEthMacCached[uLoopIndex] = arrUserMacAddr[uLoopIndex];
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL or DHCP_RETURN_INVALID
//=================================================================================
u8 uDHCPMessageValidate(struct sDHCPObject *pDHCPObjectPtr){
  /* u8 uBootpPort[] = {0x00, 0x44}; */
  u8 arrDHCPCookie[] = {0x63, 0x82, 0x53, 0x63};
  u8 uIPLen;
  u8 uIndex;
  u8 *pUserBufferPtr;

  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

  pUserBufferPtr = pDHCPObjectPtr->pUserRxBufferPtr;

  /* TODO: check for buffer overruns */

  /*adjust ip base value if ip length greater than 20 bytes*/
  uIPLen = (((pUserBufferPtr[IP_FRAME_BASE] & 0x0F) * 4) - 20);
  if (uIPLen > 40){
    return DHCP_RETURN_INVALID;
  }

/*TODO: checksum validation*/

#if 0 /* short circuit these checks since a valid dhcp magic cookie should be good enough */

  if (pUserBufferPtr[IP_FRAME_BASE + IP_PROT_OFFSET] != 0x11){                                   /*udp?*/
    return DHCP_RETURN_INVALID; /* not a udp packet */
  }

  if (memcmp(pUserBufferPtr + uIPLen + UDP_FRAME_BASE + UDP_DST_PORT_OFFSET, uBootPort, 2) != 0){   /*port 68?*/
    return DHCP_RETURN_INVALID;
  }

  if (pUserBuffer[uIPLen + BOOTP_FRAME_BASE + BOOTP_OPTYPE_OFFSET] != 0x02){                  /*bootp reply?*/
    return DHCP_RETURN_INVALID;
  }
#endif

  if (memcmp(pUserBufferPtr + uIPLen + DHCP_OPTIONS_BASE, arrDHCPCookie, 4) != 0){                 /*dhcp magic cookie?*/
    return DHCP_RETURN_INVALID;
  }

  /* check message-xid against cached-xid */
  for (uIndex = 0; uIndex < 4; uIndex++){
    if (pUserBufferPtr[uIPLen + BOOTP_FRAME_BASE + BOOTP_XID_OFFSET + uIndex] != (((pDHCPObjectPtr->uDHCPXidCached) >> (8 * (3 - uIndex))) & 0xff)){
      return DHCP_RETURN_INVALID;
    }
  }
  
  return DHCP_RETURN_OK;   /* valid reply */
}

/********** DHCP API helper functions **********/

//=================================================================================
//  uDHCPSetGotMsgFlag
//---------------------------------------------------------------------------------
//  This method is called once a valid DHCP message is found in the user rx buffer.
//  Use in conjuction with uDHCPMessageValidate().
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetGotMsgFlag(struct sDHCPObject *pDHCPObjectPtr){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }
  
  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
  
  return DHCP_RETURN_OK;
}

//=================================================================================
//  uDHCPSetStateMachineEnable
//---------------------------------------------------------------------------------
//  This method enables the DHCP state machine.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  uEnable         IN    set to SM_TRUE or SM_FALSE
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
u8 uDHCPSetStateMachineEnable(struct sDHCPObject *pDHCPObjectPtr, u8 uEnable){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  /* check if the object has been initialised */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
void vDHCPStateMachineReset(struct sDHCPObject *pDHCPObjectPtr){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return;
  }

  /* stop the state machine */
  vDHCPAuxClearFlag((u8 *)&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN);
  /* reset internal state */
  pDHCPObjectPtr->tDHCPCurrentState = INIT;
}

//=================================================================================
//  vDHCPSetHostName
//---------------------------------------------------------------------------------
//  This method sets the host name to be requested in a DHCP message.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  stringHostName  IN    pointer to host name string. Limited to HOST_NAME_MAX_SIZE.
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
/* this function only copies a max of HOST_NAME_MAX_SIZE characters */
u8 vDHCPSetHostName(struct sDHCPObject *pDHCPObjectPtr, const char *stringHostName){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }
  /* check if the object has been initialised */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }
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

/********** DHCP API event callback registration **********/

//=================================================================================
//  eventDHCPOnMsgBuilt
//---------------------------------------------------------------------------------
//  This method registers the callback to be invoked once a complete message exists
//  in the user supplied tx buffer.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  callback        IN    function pointer of type defined form tcallUserFunction
//  pDataPtr        IN    pointer to a data object to be passed to the callback
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
int eventDHCPOnMsgBuilt(struct sDHCPObject *pDHCPObjectPtr, tcallUserFunction callback, void *pDataPtr){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  /* check if the object has been initialised */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  callback        IN    function pointer of type defined form tcallUserFunction
//  pDataPtr        IN    pointer to a data object to be passed to the callback
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
int eventDHCPOnLeaseAcqd(struct sDHCPObject *pDHCPObjectPtr, tcallUserFunction callback, void *pDataPtr){
  /* check if the object exists */
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  /* check if the object has been initialised */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

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
//  Parameter	      Dir     Description
//  ---------	      ---	    -----------
//  pDHCPObjectPtr  IN      handle to DHCP state object
//
//  Return
//  ------
//  DHCP_RETURN_OK, DHCP_RETURN_FAIL or DHCP_RETURN_NOT_ENABLED
//=================================================================================
u8 uDHCPStateMachine(struct sDHCPObject *pDHCPObjectPtr){
  
  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_STATE_MACHINE_EN) == statusCLR){
    return DHCP_RETURN_NOT_ENABLED;
  }

  pDHCPObjectPtr->uDHCPTimeout++;
  pDHCPObjectPtr->uDHCPCurrentClkTick++;

  /* this logic prevents statemachine getting trapped in a particular state */
  if (pDHCPObjectPtr->tDHCPCurrentState < BOUND){
    /* after <t> seconds, if still not bound, reset state machine */
    if (pDHCPObjectPtr->uDHCPTimeout >= DHCP_SM_INTERVAL){
      pDHCPObjectPtr->tDHCPCurrentState = INIT;
      /* check if we must stop trying eventually...else keep trying */
      if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER) == statusCLR){
      /* if we've retried <n> times, quit trying */
        if (pDHCPObjectPtr->uDHCPRetries >= DHCP_SM_RETRIES){
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

  /* do the work */
  pDHCPObjectPtr->tDHCPCurrentState = dhcp_state_table[pDHCPObjectPtr->tDHCPCurrentState](pDHCPObjectPtr);

  return DHCP_RETURN_OK;
}

/*********** Internal state machine functions **********/

//=================================================================================
//  init_dhcp_state
//---------------------------------------------------------------------------------
//  This method initialises the state machine parameters (timers, flags, counters).
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  RANDOMIZE (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState init_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){

  /*initialize all statemachine parameters*/
  vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
  /* TODO implement API function uDHCPSetAutoRediscoverEnable */
  vDHCPAuxSetFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_AUTO_REDISCOVER);
  pDHCPObjectPtr->uDHCPErrors = 0;
  pDHCPObjectPtr->uDHCPInternalTimer = 0;
  pDHCPObjectPtr->uDHCPTimeout = 0;

  //FIXME pDHCPObjectPtr->uDHCPRandomWait = (u32) (uDHCPRandomNumber(0) % 40 + 10);
  //pDHCPObjectPtr->uDHCPRandomWait = 0;
  pDHCPObjectPtr->uDHCPRandomWait = DHCP_SM_WAIT; /* wait a prerequisite amount of time before dhcp'ing */

  return RANDOMIZE;
}

//=================================================================================
//  randomize_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements a randomized wait state before sending DHCP Discover msg.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  RANDOMIZE or SELECT (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState randomize_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){

  if (pDHCPObjectPtr->uDHCPRandomWait <= 0){
    pDHCPObjectPtr->uDHCPRetries++;

    if (uDHCPBuildMessage(pDHCPObjectPtr, DHCPDISCOVER, (1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_RESET_SEC | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
      /* on failure */
      pDHCPObjectPtr->uDHCPErrors++;
    } else {
      /* on success */
      pDHCPObjectPtr->uDHCPTx++;
    }
    return SELECT;
  }

  pDHCPObjectPtr->uDHCPRandomWait--;
  return RANDOMIZE;
}

//=================================================================================
//  select_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the selecting state which waits for a valid DHCP Offer msg.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  SELECT or WAIT (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState select_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){
  typeDHCPMessage tDHCPMsgType;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

    tDHCPMsgType = tDHCPProcessMsg(pDHCPObjectPtr);
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
  
  return SELECT;
}

//=================================================================================
//  wait_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements a randomized wait state before sending a DHCP Request msg.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  WAIT or REQUEST (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState wait_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){

  if (pDHCPObjectPtr->uDHCPRandomWait <= 0){
    if (uDHCPBuildMessage(pDHCPObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_DHCP_REQIP | 1<<flagDHCP_MSG_DHCP_SVRID | 1<<flagDHCP_MSG_DHCP_REQPARAM | 1<<flagDHCP_MSG_DHCP_VENDID))){
      pDHCPObjectPtr->uDHCPErrors++;
    } else {
      pDHCPObjectPtr->uDHCPTx++;
      return REQUEST;
    }
  }
  
  pDHCPObjectPtr->uDHCPRandomWait--;
  return WAIT;
}

//=================================================================================
//  request_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the requesting state which waits for a valid DHCP Ack msg.
//  It sets the various lease parameters and calls the user specified callback once
//  the lease has been obtained.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  REQUEST or BOUND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState request_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){
  typeDHCPMessage tDHCPMsgType;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);

    tDHCPMsgType = tDHCPProcessMsg(pDHCPObjectPtr);
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
        (*(pDHCPObjectPtr->callbackOnLeaseAcqd))(pDHCPObjectPtr, pDHCPObjectPtr->pLeaseDataPtr);
      }

      return BOUND;
    } else {
      pDHCPObjectPtr->uDHCPErrors++;
    } 
  }

  return REQUEST;
}

//=================================================================================
//  bound_dhcp_state
//---------------------------------------------------------------------------------
//  This method implements the bound state which waits the prerequisite amount of 
//  time before attempting to renew the lease.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  BOUND or RENEW (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState bound_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){
  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if ((pDHCPObjectPtr->uDHCPInternalTimer * POLL_INTERVAL) >= (pDHCPObjectPtr->uDHCPT1 * 1000)){
    /* time to renew our lease */
    if (uDHCPBuildMessage(pDHCPObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_UNICAST | 1<<flagDHCP_MSG_BOOTP_CIPADDR | 1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_RESET_SEC | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
      pDHCPObjectPtr->uDHCPErrors++;
    } else {
      pDHCPObjectPtr->uDHCPTx++;
    }
    return RENEW;
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  BOUND or RENEW or REBIND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState renew_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){
  typeDHCPMessage tDHCPMsgType;

  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
    
    tDHCPMsgType = tDHCPProcessMsg(pDHCPObjectPtr);
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
    if (uDHCPBuildMessage(pDHCPObjectPtr, DHCPREQUEST, (1<<flagDHCP_MSG_BOOTP_CIPADDR | 1<<flagDHCP_MSG_DHCP_VENDID | 1<<flagDHCP_MSG_DHCP_NEW_XID))){
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  BOUND or RANDOMIZE or REBIND (enumerated typeDHCPState, the next state to be entered)
//=================================================================================
static typeDHCPState rebind_dhcp_state(struct sDHCPObject *pDHCPObjectPtr){
  typeDHCPMessage tDHCPMsgType;

  /*  advance the DHCP state machine timer / counter  */
  pDHCPObjectPtr->uDHCPInternalTimer++;

  if (tDHCPAuxTestFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE) == statusSET){
    vDHCPAuxClearFlag(&(pDHCPObjectPtr->uDHCPRegisterFlags), flagDHCP_SM_GOT_MESSAGE);
    tDHCPMsgType = tDHCPProcessMsg(pDHCPObjectPtr);
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//  tDHCPMsgType    IN    DHCP message type (ennumerated type)
//  tDHCPMsgFlags   IN    flags to set various message parameters (enumerated type)
//
//  Return
//  ------
//  DHCP_RETURN_OK or DHCP_RETURN_FAIL
//=================================================================================
static u8 uDHCPBuildMessage(struct sDHCPObject *pDHCPObjectPtr, typeDHCPMessage tDHCPMsgType, typeDHCPMessageFlags tDHCPMsgFlags){
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

  if (pDHCPObjectPtr == NULL){
    return DHCP_RETURN_FAIL;
  }

  /* has uDHCPInit been called to initialize dependancies? */
  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return DHCP_RETURN_FAIL;
  }

  /* simplify ur life */
  pBuffer = pDHCPObjectPtr->pUserTxBufferPtr;

  if (pBuffer == NULL){
    return DHCP_RETURN_FAIL;
  }

  uSize = pDHCPObjectPtr->uUserTxBufferSize;

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
  memcpy(pBuffer + ETH_SRC_OFFSET, pDHCPObjectPtr->arrDHCPEthMacCached, 6);

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
    uTempXid = ((pDHCPObjectPtr->arrDHCPEthMacCached[2] & 0x0f) << 28) + \
               ((pDHCPObjectPtr->arrDHCPEthMacCached[3] & 0x0f) << 24) + \
               ((pDHCPObjectPtr->arrDHCPEthMacCached[4] & 0x0f) << 20) + \
               ((pDHCPObjectPtr->arrDHCPEthMacCached[5] & 0x0f) << 16);
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

  memcpy(pBuffer + BOOTP_FRAME_BASE + BOOTP_CHWADDR_OFFSET, pDHCPObjectPtr->arrDHCPEthMacCached, 6);

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
  memcpy(pBuffer + DHCP_OPTIONS_BASE + uIndex, pDHCPObjectPtr->arrDHCPEthMacCached, 6);
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
  uLength = UDP_FRAME_TOTAL_LEN + BOOTP_FRAME_TOTAL_LEN + uIndex;
  pBuffer[UDP_FRAME_BASE + UDP_ULEN_OFFSET    ] = (u8) ((uLength & 0xff00) >> 8);
  pBuffer[UDP_FRAME_BASE + UDP_ULEN_OFFSET + 1] = (u8) (uLength & 0xff);

  /* calculate and fill in ip frame packet lengths */
  uLength = uLength + IP_FRAME_TOTAL_LEN;
  pBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET    ] = (u8) ((uLength & 0xff00) >> 8);
  pBuffer[IP_FRAME_BASE + IP_TLEN_OFFSET + 1] = (u8) (uLength & 0xff);

  /* calculate checksums - ip mandatory, udp optional */
  uChecksum = uChecksum16Calc(pBuffer, IP_FRAME_BASE, UDP_FRAME_BASE - 1);
  pBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET    ] = (u8) ((uChecksum & 0xff00) >> 8);
  pBuffer[IP_FRAME_BASE + IP_CHKSM_OFFSET + 1] = (u8) (uChecksum & 0xff);

  /* TODO: calculate udp checksum value */


  /* pad to nearest 64 bit boundary */
  /* simply increase total length by following amount - these bytes already zero due to earlier memset */
  uPaddingLength = 8 - ((uLength + ETH_FRAME_TOTAL_LEN) % 8);
  
  /* and with 0b111 since only interested in values of 0 to 7 */
  uPaddingLength = uPaddingLength & 0x7; 

  pDHCPObjectPtr->uDHCPMsgSize = uLength + ETH_FRAME_TOTAL_LEN + uPaddingLength;

  /* TODO: set a message ready flag */

  /* invoke a callback once the message successfully */
  if (pDHCPObjectPtr->callbackOnMsgBuilt != NULL){
    (*(pDHCPObjectPtr->callbackOnMsgBuilt))(pDHCPObjectPtr, pDHCPObjectPtr->pMsgDataPtr);
  }

  return DHCP_RETURN_OK;
}

//=================================================================================
//  tDHCPProcessMsg
//---------------------------------------------------------------------------------
//  This method processes a valid DHCP message located in the user supplied rx buffer
//  and extracts all the necessary lease parameters.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDHCPObjectPtr  IN    handle to DHCP state object
//
//  Return
//  ------
//  typeDHCPMessage (enumerated type)
//=================================================================================
static typeDHCPMessage tDHCPProcessMsg(struct sDHCPObject *pDHCPObjectPtr){
  u16 uOptionIndex = 0;
  u16 uOptionEnd = 0;
  u8 uOption = 0;
  u8 uTmpIndex = 0;

  u8 uIPLen = 0;
  u8 *pBuffer;
  u32 uDHCPTmpXid = 0;
  typeDHCPMessage tDHCPMsgType = DHCPERROR;

  if (pDHCPObjectPtr == NULL){
    return DHCPERROR;
  }

  /* simplify your life */
  pBuffer = pDHCPObjectPtr->pUserRxBufferPtr;

  if (pBuffer == NULL){
    return DHCPERROR;
  }

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

  return tDHCPMsgType;
}

/**********  Some auxiliary functions **********/

//=================================================================================
//  uChecksum16Calc
//---------------------------------------------------------------------------------
//  This method calculates a 16-bit word checksum.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pDataPtr        IN    pointer to buffer with data to be "checksummed"
//  uIndexStart     IN    starting index in the data buffer
//  uIndexEnd       IN    ending index in the data buffer
//
//  Return
//  ------
//  0xffff or valid checksum
//=================================================================================
static u16 uChecksum16Calc(u8 *pDataPtr, u16 uIndexStart, u16 uIndexEnd){
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
//  uDHCPRandomNumber
//---------------------------------------------------------------------------------
//  This method generates a pseudo "random" number.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
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
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
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
