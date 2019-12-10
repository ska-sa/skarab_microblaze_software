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

/* TODO: if NUM_ETHERNET_INTERFACES > 17 -> throw compile-time/preprocessor error */
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

  pIGMPObjectPtr->uIGMPMulticastAddress = 0;
  pIGMPObjectPtr->uIGMPMulticastAddressMask = 0xffffffff;

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

  if (uId >= NUM_ETHERNET_INTERFACES){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Interface ID %02d out of range\r\n", uId);
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error? */

  pIGMPObjectPtr->uIGMPJoinRequestFlag = TRUE;

  pIGMPObjectPtr->uIGMPMulticastAddress = uMulticastBaseAddr;
  pIGMPObjectPtr->uIGMPMulticastAddressMask = uMulticastAddrMask;

  return XST_SUCCESS;
}

/*FIXME roll leave and join flag into one variable */

u8 uIGMPLeaveGroup(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;

  if (uId >= NUM_ETHERNET_INTERFACES){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Interface ID %02d out of range\r\n", uId);
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error? */

  pIGMPObjectPtr->uIGMPLeaveRequestFlag = TRUE;

  return XST_SUCCESS;
}

u8 uIGMPRejoinPrevGroup(u8 uId){
  struct sIGMPObject *pIGMPObjectPtr;

  if (uId >= NUM_ETHERNET_INTERFACES){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [..] Interface ID %02d out of range\r\n", uId);
    return XST_FAILURE;
  }

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);    /* TODO: should we assert here or throw a runtime error? */

  if ((pIGMPObjectPtr->tIGMPCurrentState == IGMP_JOINED_STATE) || (pIGMPObjectPtr->tIGMPCurrentState == IGMP_SEND_MEMBERSHIP_REPORTS_STATE)){
    pIGMPObjectPtr->uIGMPJoinRequestFlag = TRUE;
    pIGMPObjectPtr->tIGMPCurrentState = IGMP_IDLE_STATE;
  }

  return XST_SUCCESS;
}

/*-------------------IGMP state machine and its internal functions--------------------------------*/
u8 uIGMPStateMachine(u8 uId)
{
  struct sIGMPObject *pIGMPObjectPtr;

  SANE_IGMP_IF_ID(uId);

  pIGMPObjectPtr = pIGMPGetContext(uId);

  SANE_IGMP(pIGMPObjectPtr);

  log_printf(LOG_SELECT_IGMP, LOG_LEVEL_TRACE, "IGMP [%02d] entering state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[pIGMPObjectPtr->tIGMPCurrentState]);

  pIGMPObjectPtr->tIGMPCurrentState = igmp_state_table[pIGMPObjectPtr->tIGMPCurrentState](pIGMPObjectPtr);

  return XST_SUCCESS;
}


static typeIGMPState igmp_idle_state(struct sIGMPObject *pIGMPObjectPtr)
{
  pIGMPObjectPtr->uIGMPCurrentMessage = 0;
  pIGMPObjectPtr->uIGMPCurrentClkTick = 0;
  pIGMPObjectPtr->uIGMPLeaveRequestFlag = FALSE;

  if (pIGMPObjectPtr->uIGMPJoinRequestFlag == TRUE){
    /* clear the join request flag here to prevent ejoining once a leave is requested later creating a cycle */
    pIGMPObjectPtr->uIGMPJoinRequestFlag = FALSE;
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[IGMP_SEND_MEMBERSHIP_REPORTS_STATE]);
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
  if (pIGMPObjectPtr->uIGMPCurrentMessage > (~((unsigned) pIGMPObjectPtr->uIGMPMulticastAddressMask))){
    /* pIGMPObjectPtr->uIGMPCurrentMessage = 0; */
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[IGMP_JOINED_STATE]);
    return IGMP_JOINED_STATE;
  }

  baseip = pIGMPObjectPtr->uIGMPMulticastAddress;
  mask = pIGMPObjectPtr->uIGMPMulticastAddressMask;

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
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[IGMP_SEND_MEMBERSHIP_REPORTS_STATE]);
    return IGMP_SEND_MEMBERSHIP_REPORTS_STATE;
  }

  if (pIGMPObjectPtr->uIGMPLeaveRequestFlag == TRUE){
    pIGMPObjectPtr->uIGMPLeaveRequestFlag = FALSE;
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[IGMP_LEAVING_STATE]);
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
  if (pIGMPObjectPtr->uIGMPCurrentMessage > (~((unsigned) pIGMPObjectPtr->uIGMPMulticastAddressMask))){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_DEBUG, "IGMP [%02d] about to enter state %s\r\n", pIGMPObjectPtr->uIGMPIfId, igmp_state_name_lookup[IGMP_IDLE_STATE]);
    return IGMP_IDLE_STATE;
  }

  baseip = pIGMPObjectPtr->uIGMPMulticastAddress;
  mask = pIGMPObjectPtr->uIGMPMulticastAddressMask;

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
  pIGMPObjectPtr->uIGMPJoinRequestFlag = FALSE;

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
