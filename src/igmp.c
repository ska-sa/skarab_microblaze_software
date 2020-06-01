/*
 *  RvW (SARAO) - Dec 2019
 */

/* Xilinx lib includes */
#include <xstatus.h>
#include <xil_types.h>

#include "igmp.h"
#include "if.h"
#include "logging.h"
#include "constant_defs.h"
#include "eth_sorter.h"
#include "eth_mac.h"

typedef typeIGMPState (*igmp_state_func_ptr)(struct sIGMPObject *pIGMPObjectPtr);

static typeIGMPState igmp_idle_state(struct sIGMPObject *pIGMPObjectPtr);
static typeIGMPState igmp_send_membership_reports_state(struct sIGMPObject *pIGMPObjectPtr);
static typeIGMPState igmp_joined_state(struct sIGMPObject *pIGMPObjectPtr);
static typeIGMPState igmp_leaving_state(struct sIGMPObject *pIGMPObjectPtr);

static igmp_state_func_ptr igmp_state_table[] = {
  [IGMP_IDLE_STATE]                     = igmp_idle_state,
  [IGMP_SEND_MEMBERSHIP_REPORTS_STATE]  = igmp_send_membership_reports_state,
  [IGMP_JOINED_STATE]                   = igmp_joined_state,
  [IGMP_LEAVING_STATE]                  = igmp_leaving_state
};

static const char *igmp_state_name_lookup[] = {
  [IGMP_IDLE_STATE]                     = "igmp_idle_state",
  [IGMP_SEND_MEMBERSHIP_REPORTS_STATE]  = "igmp_send_membership_reports_state",
  [IGMP_JOINED_STATE]                   = "igmp_joined_state",
  [IGMP_LEAVING_STATE]                  = "igmp_leaving_state"
};

static struct sIGMPObject *pIGMPGetContext(u8 uId);

/*TODO this should be a tunable and configurable parameter */
#define IGMP_REPORT_TIMER 600
#define IGMP_SM_POLL_INTERVAL 100

#define IGMP_MAGIC_CANARY   0xB17D5EED

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_IGMP(IGMPObject) __sanity_check_igmp(IGMPObject);
#define SANE_IGMP_IF_ID(id)    __sanity_check_igmp_interface_id(id);
#else
#define SANE_IGMP(IGMPObject)
#define SANE_IGMP_IF_ID(id)
#endif

#ifdef DO_SANITY_CHECKS
static void __sanity_check_igmp(struct sIGMPObject *pIGMPObjectPtr){
  Xil_AssertVoid(NULL != pIGMPObjectPtr);    /* API usage error */
  Xil_AssertVoid(IGMP_MAGIC_CANARY == pIGMPObjectPtr->uIGMPMagic);    /* API usage error */
}

/* TODO: use get_num_interfaces() */
static void __sanity_check_igmp_interface_id(u8 if_id){
  Xil_AssertVoid((if_id >= 0) && (if_id < NUM_ETHERNET_INTERFACES));    /* API usage error */
}
#endif

/*----------------------IGMP API Functions----------------------------------------------*/

struct sIGMPObject *pIGMPInit(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;

  SANE_IGMP_IF_ID(uId);

  pIGMPObjectPtr = pIGMPGetContext(uId);

  pIGMPObjectPtr->uIGMPMagic = IGMP_MAGIC_CANARY;

  pIGMPObjectPtr->tIGMPCurrentState = IGMP_IDLE_STATE;

  pIGMPObjectPtr->uIGMPCurrentMessage = 0;
  pIGMPObjectPtr->uIGMPCurrentClkTick = 0;
  pIGMPObjectPtr->uIGMPIfId = uId;

  /* FIXME - double check the assignment of join and leave mc addr and mask throughout the s/m */
  pIGMPObjectPtr->uIGMPJoinMulticastAddress = 0;
  pIGMPObjectPtr->uIGMPJoinMulticastAddressMask = 0xffffffff;

  pIGMPObjectPtr->uIGMPLeaveMulticastAddress = 0;
  pIGMPObjectPtr->uIGMPLeaveMulticastAddressMask = 0xffffffff;

  pIGMPObjectPtr->uIGMPJoinRequestFlag = FALSE;
  pIGMPObjectPtr->uIGMPLeaveRequestFlag = FALSE;

  return pIGMPObjectPtr;
}


static struct sIGMPObject *pIGMPGetContext(u8 uId){
  static struct sIGMPObject IGMPContext[NUM_ETHERNET_INTERFACES];  /* theoretically, we could have up to 16 i/f's */

  SANE_IGMP_IF_ID(uId);

  return &(IGMPContext[uId]);
}

/* the following functions with their args can be invoked by user commands outside of the API programming context and
 * therefore it's not a good idea to assert() args inside these functions
 */

u8 uIGMPJoinGroup(u8 uId, u32 uMulticastBaseAddr, u32 uMulticastAddrMask){
  struct sIGMPObject *pIGMPObjectPtr;

  if (IF_ID_PRESENT != check_interface_valid(uId)) {
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Multicast join failed\r\n");
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error? */

  /* TODO: Q: should we check for sane ip and mask? e.g. it may not be useful to invoke the join state with a mask of 0xffffffff
           A: there may actually be a use case for a mask of 0xffffffff -> when you want to subscribe
               to only one specific addr, this will do it */
          /* however, perhaps we should guard against extra-ordinarily large  groups, like a mask of 0xffff0000*/

  /* only set mc parameters and invoke s/m if there's a change... */
  if ((pIGMPObjectPtr->uIGMPJoinMulticastAddress != uMulticastBaseAddr) ||
     (pIGMPObjectPtr->uIGMPJoinMulticastAddressMask != uMulticastAddrMask )){

    pIGMPObjectPtr->uIGMPLeaveMulticastAddress = pIGMPObjectPtr->uIGMPJoinMulticastAddress;
    pIGMPObjectPtr->uIGMPLeaveMulticastAddressMask = pIGMPObjectPtr->uIGMPJoinMulticastAddressMask;

    pIGMPObjectPtr->uIGMPJoinMulticastAddress = uMulticastBaseAddr;
    pIGMPObjectPtr->uIGMPJoinMulticastAddressMask = uMulticastAddrMask;

    pIGMPObjectPtr->uIGMPJoinRequestFlag = TRUE;
    /*pIGMPObjectPtr->tIGMPCurrentState = IGMP_IDLE_STATE;*/
  } else {
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_INFO, "IGMP [%02d] Already subscribed with base %08x and mask %08x\r\n", uId, uMulticastBaseAddr,uMulticastAddrMask);
  }

  return XST_SUCCESS;
}

/* TODO roll leave and join flag into one variable */

u8 uIGMPLeaveGroup(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;

  log_printf(LOG_SELECT_IGMP, LOG_LEVEL_INFO, "IGMP [..] Leaving groups on if-%d\r\n", uId);

  if (IF_ID_PRESENT != check_interface_valid(uId)) {
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Multicast leave failed\r\n");
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error to the console? Bearing in mind
                                 that this is a function called from a user command but also that the ptr should have
                                 been initialised first i.e. an assert would catch this in development... */

  pIGMPObjectPtr->uIGMPLeaveRequestFlag = TRUE;

  pIGMPObjectPtr->uIGMPLeaveMulticastAddress = pIGMPObjectPtr->uIGMPJoinMulticastAddress;
  pIGMPObjectPtr->uIGMPLeaveMulticastAddressMask = pIGMPObjectPtr->uIGMPJoinMulticastAddressMask;

  pIGMPObjectPtr->uIGMPJoinMulticastAddress = 0;
  pIGMPObjectPtr->uIGMPJoinMulticastAddressMask = 0xffffffff;

  return XST_SUCCESS;
}


u8 uIGMPLeaveGroupFlush(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;
  int ret;

  ret = uIGMPLeaveGroup(uId);
  if (XST_FAILURE == ret){
    return XST_FAILURE;
  }

  /*
   * now run the igmp state machine until all the leave messages have been flushed
   * This function should block till all the leave messages have been sent.
   * i.e. until the state machine has returned to the idle state from leave state
   */
  pIGMPObjectPtr = pIGMPGetContext(uId);

  while (pIGMPObjectPtr->tIGMPCurrentState != IGMP_IDLE_STATE){
    uIGMPStateMachine(uId);
  }

  return XST_SUCCESS;
}



/* This function was introduced to deal with link-flap scenarios and rejoin the mc group */
u8 uIGMPRejoinPrevGroup(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;

  if (IF_ID_PRESENT != check_interface_valid(uId)) {
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Multicast rejoin failed\r\n");
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error? same reason as above... */

  if ((pIGMPObjectPtr->tIGMPCurrentState == IGMP_JOINED_STATE) || (pIGMPObjectPtr->tIGMPCurrentState == IGMP_SEND_MEMBERSHIP_REPORTS_STATE)){
    pIGMPObjectPtr->uIGMPJoinRequestFlag = TRUE;
    pIGMPObjectPtr->tIGMPCurrentState = IGMP_IDLE_STATE;
  }

  return XST_SUCCESS;
}


void vIGMPPrintInfo(void){
  u8 num_links;
  u8 log_id;  /* logical id */
  u8 phy_id;  /* physical id*/
  u32 base_addr;
  u32 mask_addr;
  typeIGMPState current_state;
  u8 cached_id;
  struct sIGMPObject *pIGMPObjectPtr;

  num_links = get_num_interfaces();
  for (log_id = 0; log_id < num_links; log_id++){
    phy_id = get_interface_id(log_id);
    pIGMPObjectPtr = pIGMPGetContext(phy_id);

    base_addr = pIGMPObjectPtr->uIGMPJoinMulticastAddress;
    mask_addr = pIGMPObjectPtr->uIGMPJoinMulticastAddressMask;
    cached_id = pIGMPObjectPtr->uIGMPIfId;   /* included as cross-check to verify that the id mapping is correct */
    current_state = pIGMPObjectPtr->tIGMPCurrentState;

    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_INFO, "IGMP [%02d] id: %02d base: %08x mask: %08x state: %s\r\n", phy_id,
        cached_id, base_addr, mask_addr, igmp_state_name_lookup[current_state]);
  }
}

/*-------------------IGMP state machine and its internal functions--------------------------------*/
u8 uIGMPStateMachine(u8 uId)
{
  struct sIGMPObject *pIGMPObjectPtr;
  typeIGMPState tIGMPNextState;   /* temp intermediate state variable to evaluate a state change */

  SANE_IGMP_IF_ID(uId);

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);

  log_printf(LOG_SELECT_IGMP, LOG_LEVEL_TRACE, "IGMP [%02d] entering state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[pIGMPObjectPtr->tIGMPCurrentState]);

  tIGMPNextState = igmp_state_table[pIGMPObjectPtr->tIGMPCurrentState](pIGMPObjectPtr);

  /* only upon a state change, print debug info... */
  if (tIGMPNextState != pIGMPObjectPtr->tIGMPCurrentState){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[tIGMPNextState]);
  }

  /* update the state variable */
  pIGMPObjectPtr->tIGMPCurrentState = tIGMPNextState;

  return XST_SUCCESS;
}


static typeIGMPState igmp_idle_state(struct sIGMPObject *pIGMPObjectPtr)
{
  pIGMPObjectPtr->uIGMPCurrentMessage = 0;
  pIGMPObjectPtr->uIGMPCurrentClkTick = 0;
  pIGMPObjectPtr->uIGMPLeaveRequestFlag = FALSE;

  if (pIGMPObjectPtr->uIGMPJoinRequestFlag == TRUE){
    /* clear the join request flag here to prevent rejoining once a leave is requested later, creating a cycle */
    pIGMPObjectPtr->uIGMPJoinRequestFlag = FALSE;
    return IGMP_SEND_MEMBERSHIP_REPORTS_STATE;
  }

  return IGMP_IDLE_STATE;
}


static typeIGMPState igmp_send_membership_reports_state(struct sIGMPObject *pIGMPObjectPtr){
  struct sIFObject *ifptr;
  int iStatus;
  u8 id;
  u32 groupaddr;
  u32 baseip;
  u32 mask;
  u32 pktlen;
  u8 *txbuf;

  /* have we cycled through all the masked addresses? */
  if (pIGMPObjectPtr->uIGMPCurrentMessage > (~((unsigned) pIGMPObjectPtr->uIGMPJoinMulticastAddressMask))){
    /* pIGMPObjectPtr->uIGMPCurrentMessage = 0; */
    return IGMP_JOINED_STATE;
  }

  baseip = pIGMPObjectPtr->uIGMPJoinMulticastAddress;
  mask = pIGMPObjectPtr->uIGMPJoinMulticastAddressMask;

  groupaddr = (baseip & mask) | pIGMPObjectPtr->uIGMPCurrentMessage;

  id = pIGMPObjectPtr->uIGMPIfId;
  ifptr = lookup_if_handle_by_id(id);
  txbuf = ifptr->pUserTxBufferPtr;    /* reaching into the ifptr data struct here. Should possibly hide this in an
                                         abstraction or a wrapper in future */

  CreateIGMPPacket(id, txbuf, &pktlen, IGMP_MEMBERSHIP_REPORT, groupaddr);
  pktlen = (pktlen >> 2);
  iStatus =  TransmitHostPacket(id, (u32 *) txbuf, pktlen);

  if (iStatus != XST_SUCCESS){
    IFCounterIncr(ifptr, TX_IP_IGMP_ERR);
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] FAILED to send membership report for ip %08x\r\n", id, groupaddr);
  } else {
    IFCounterIncr(ifptr, TX_IP_IGMP_OK);
    /* log-level set to WARN in order to display during programming */
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_WARN, "IGMP [%02d] sent membership report for ip %08x\r\n", id, groupaddr);
  }

  pIGMPObjectPtr->uIGMPCurrentMessage++;

  return IGMP_SEND_MEMBERSHIP_REPORTS_STATE;
}


static typeIGMPState igmp_joined_state(struct sIGMPObject *pIGMPObjectPtr)
{

  pIGMPObjectPtr->uIGMPCurrentClkTick++;
  pIGMPObjectPtr->uIGMPCurrentMessage = 0;

  /* TODO: SANE_IGMP_TICK() - check that the igmp state will actually fire since timeout may be larger than sizeof(tick) */

  /* is it time to send IGMP reports to the router/switch? */
  if (pIGMPObjectPtr->uIGMPCurrentClkTick >= IGMP_REPORT_TIMER) {
    pIGMPObjectPtr->uIGMPCurrentClkTick = 0;
    return IGMP_SEND_MEMBERSHIP_REPORTS_STATE;
  }

  if (pIGMPObjectPtr->uIGMPLeaveRequestFlag == TRUE){
    pIGMPObjectPtr->uIGMPLeaveRequestFlag = FALSE;
    return IGMP_LEAVING_STATE;
  }

  /* if the uIGMPJoinRequestFlag flag is set while in 'joined state', a new multicast configuration (addr and mask) was
   * most likely requested. We have to leave current group and re-issue new igmp join requests...  */
  if (pIGMPObjectPtr->uIGMPJoinRequestFlag == TRUE){
    /* NOTE DO NOT clear the join request flag here since we want the s/m to filter back to the joined state after the
     * next leave state and this flag will allow us to do this when we get back to 'idle state'. */
    return IGMP_LEAVING_STATE;
  }

  return IGMP_JOINED_STATE;
}


static typeIGMPState igmp_leaving_state(struct sIGMPObject *pIGMPObjectPtr){
  struct sIFObject *ifptr;
  int iStatus;
  u8 id;
  u32 groupaddr;
  u32 baseip;
  u32 mask;
  u32 pktlen;
  u8 *txbuf;

  /* have we unsubscribed from all multicast group addresses? */
  if (pIGMPObjectPtr->uIGMPCurrentMessage > (~((unsigned) pIGMPObjectPtr->uIGMPLeaveMulticastAddressMask))){
    return IGMP_IDLE_STATE;
  }

  baseip = pIGMPObjectPtr->uIGMPLeaveMulticastAddress;
  mask = pIGMPObjectPtr->uIGMPLeaveMulticastAddressMask;

  groupaddr = (baseip & mask) | pIGMPObjectPtr->uIGMPCurrentMessage;

  id = pIGMPObjectPtr->uIGMPIfId;
  ifptr = lookup_if_handle_by_id(id);
  txbuf = ifptr->pUserTxBufferPtr;    /* reaching into the ifptr data struct here. Should possibly hide this in an
                                         abstraction or a wrapper in future */

  CreateIGMPPacket(id, txbuf, &pktlen, IGMP_LEAVE_REPORT, groupaddr);
  pktlen = (pktlen >> 2);
  iStatus =  TransmitHostPacket(id, (u32 *) txbuf, pktlen);

  if (iStatus != XST_SUCCESS){
    IFCounterIncr(ifptr, TX_IP_IGMP_ERR);
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] FAILED to send leave report for ip %08x\r\n", id, groupaddr);
  } else {
    IFCounterIncr(ifptr, TX_IP_IGMP_OK);
    /* log-level set to WARN in order to display during programming */
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_WARN, "IGMP [%02d] sent leave report for ip %08x\r\n", id, groupaddr);
  }

  pIGMPObjectPtr->uIGMPCurrentMessage++;

  return IGMP_LEAVING_STATE;
}



/*----------------------------------private functions----------------------------------------------*/


#if 0
static u8 uIGMPBuildMessage(struct sIGMPObject *pIGMPObjectPtr, typeIGMPMessage tIGMPMsgType)
{

  u8 *pBuffer;
  u32 uTempAddress;

  /* simplify */
  pBuffer = pIGMPObjectPtr->uUserTxBufferPtr;

  /*****  ethernet frame stuff  *****/

  pBuffer[ETH_DST_OFFSET    ] = 0x01;
  pBuffer[ETH_DST_OFFSET + 1] = 0x00;
  pBuffer[ETH_DST_OFFSET + 2] = 0x5e;
  pBuffer[ETH_DST_OFFSET + 3] =
    pBuffer[ETH_DST_OFFSET + 4] =
      pBuffer[ETH_DST_OFFSET + 5] =


        /* fill in our hardware mac address */
        memcpy(pBuffer + ETH_SRC_OFFSET, pIGMPObjectPtr->arrIGMPEthMacCached, 6);

  /* ethernet frame type */
  pBuffer[ETH_FRAME_TYPE_OFFSET] = 0x08;

  /*****  ip frame struff  *****/
  pBuffer[IP_FRAME_BASE + IP_FLAG_FRAG_OFFSET] = 0x40;    /* ip flags = don't fragment */
  pBuffer[IP_FRAME_BASE + IP_V_HIL_OFFSET] = 0x45;

  pBuffer[IP_FRAME_BASE + IP_TTL_OFFSET] = 0x80;
  pBuffer[IP_FRAME_BASE + IP_PROT_OFFSET] = 0x11;

  /* if unicast, fill in relevant addresses else broadcast */
  if ((tDHCPAuxTestFlag( (u8 *) &(tDHCPMsgFlags), flagDHCP_MSG_UNICAST) == statusSET) && \
      ((pDHCPObjectPtr->arrDHCPAddrServerCached[0] != '\0') && (pDHCPObjectPtr->arrDHCPAddrYIPCached[0] != '\0'))) {
    memcpy(pBuffer + IP_FRAME_BASE + IP_DST_OFFSET, pDHCPObjectPtr->arrDHCPAddrServerCached, 4);
    memcpy(pBuffer + IP_FRAME_BASE + IP_SRC_OFFSET, pDHCPObjectPtr->arrDHCPAddrYIPCached, 4);
  } else {
    memset(pBuffer + IP_FRAME_BASE + IP_DST_OFFSET, 0xff, 4);     /* broadcast */
    pBuffer[BOOTP_FRAME_BASE + BOOTP_FLAGS_OFFSET] = 0x80;        /* set the broadcast bit flag in the bootp/dhcp payload */
  }

  /*****  udp frame struff  *****/
  pBuffer[UDP_FRAME_BASE + UDP_SRC_PORT_OFFSET + 1] = 0x44;
  pBuffer[UDP_FRAME_BASE + UDP_DST_PORT_OFFSET + 1] = 0x43;

}

#endif
