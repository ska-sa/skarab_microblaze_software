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
#include <xil_types.h>

#include "if.h"
#include "register.h"
#include "net_utils.h"
#include "logging.h"
#include "constant_defs.h"

/*********** Sanity Checks ***************/
#ifdef DO_SANITY_CHECKS
#define SANE_IF(IFObject) __sanity_check_interface(IFObject);
#define SANE_IF_ID(id)    __sanity_check_interface_id(id);
#else
#define SANE_IF(IFObject)
#define SANE_IF_ID(id)
#endif

#ifdef DO_SANITY_CHECKS
static void __sanity_check_interface(struct sIFObject *pIFObjectPtr){
  Xil_AssertVoid(NULL != pIFObjectPtr);    /* API usage error */
  Xil_AssertVoid(IF_MAGIC == pIFObjectPtr->uIFMagic);    /* API usage error */
}

/* TODO: if NUM_ETHERNET_INTERFACES > 17 -> throw compile-time/preprocessor error */
static void __sanity_check_interface_id(u8 if_id){
  Xil_AssertVoid(if_id < NUM_ETHERNET_INTERFACES);    /* API usage error */
}
#endif


/**************** API ********************/

struct sIFObject *InterfaceInit(u8 uEthernetId, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize, u8 *arrUserMacAddr){
  u8 uLoopIndex;
  struct sIFObject *pIFObjectPtr;

#if 0
  if (pIFObjectPtr == NULL){
    return IF_RETURN_FAIL;
  }
#endif

  SANE_IF_ID(uEthernetId);

  pIFObjectPtr = lookup_if_handle_by_id(uEthernetId);

  if (IF_MAGIC == pIFObjectPtr->uIFMagic){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "I/F  [%02x] Failed - attempted to overwrite previous state.", uEthernetId);
    return pIFObjectPtr;
  }

#ifdef DO_SANITY_CHECKS
  Xil_AssertNonvoid(pRxBufferPtr != NULL);
  Xil_AssertNonvoid(pTxBufferPtr != NULL);
  Xil_AssertNonvoid(arrUserMacAddr != NULL);

  /* check that user supplied a reasonable amount of buffer space */
  Xil_AssertNonvoid(uRxBufferSize >= IF_RX_MIN_BUFFER_SIZE);
  Xil_AssertNonvoid(uTxBufferSize >= IF_TX_MIN_BUFFER_SIZE);
#endif

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

  pIFObjectPtr->uIFAddrMask = 0;
  pIFObjectPtr->uIFAddrIP = 0;

  if (uDHCPInit(pIFObjectPtr) != DHCP_RETURN_OK){
    return NULL;
  }
  pIFObjectPtr->uIFEnableArpRequests = ARP_REQUESTS_DISABLE;
  pIFObjectPtr->uIFCurrentArpRequest = 1;

  pIFObjectPtr->uIFEthernetSubnet = 0;
  pIFObjectPtr->uIFEthernetId = uEthernetId;
  pIFObjectPtr->uIFLinkStatus = LINK_DOWN;
  pIFObjectPtr->uIFLinkRxActive = 0;

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

  return pIFObjectPtr;
}



//=================================================================================
//  UpdateEthernetLinkUpStatus
//--------------------------------------------------------------------------------
//  This method updates the Ethernet link up status. The Ethernet interface is not
//  checked for received messages if it is not up. When the Ethernet link comes up,
//  the ARP packet requests are enabled to populate the ARP cache.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF State Object
//
//  Return
//  ------
//  None
//=================================================================================
void UpdateEthernetLinkUpStatus(struct sIFObject *pIFObjectPtr){
  u32 uReg;
  u32 uMask = 0;
  u32 uActivityMask = 0;
  u8 uId;

  SANE_IF(pIFObjectPtr);

  uReg = ReadBoardRegister(C_RD_ETH_IF_LINK_UP_ADDR);
  uId = pIFObjectPtr->uIFEthernetId;
  uMask = 1 << uId;

  if ((uReg & uMask) != LINK_DOWN){
    // Check if the link was previously down
    if (pIFObjectPtr->uIFLinkStatus == LINK_DOWN){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] LINK %x HAS COME UP!\r\n", uId, uId);

      if (uId == 0){  /* 1gbe i/f */
        /* do not enable dhcp in loopback mode */
#ifndef DO_1GBE_LOOPBACK_TEST
        vDHCPStateMachineReset(pIFObjectPtr);
        uDHCPSetStateMachineEnable(pIFObjectPtr, SM_TRUE);
#endif
      } else {    /* for all other i/f's i.e. the 40gbe i/f's */
        /* do not enable dhcp in loopback mode */
#ifndef DO_40GBE_LOOPBACK_TEST
        vDHCPStateMachineReset(pIFObjectPtr);
        uDHCPSetStateMachineEnable(pIFObjectPtr, SM_TRUE);
#endif
      }
    }

    /* check for link activity - only available on 40gbe links */
    if (uId != 0){
      /*
       * report the first time link rx activity is detected - this gives us a
       * fairly good indication of when the switch's link becomes ready.
       */
      if (pIFObjectPtr->uIFLinkRxActive == 0){
        uActivityMask = 1 << (15 + (4*uId));
        if (uReg & uActivityMask){
          pIFObjectPtr->uIFLinkRxActive = 1;
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] LINK %x RX ACTIVITY!\r\n", uId, uId);
        }
      }
    }

    pIFObjectPtr->uIFLinkStatus = LINK_UP;
  } else {
    // Check if the link was previously up
    if (pIFObjectPtr->uIFLinkStatus == LINK_UP){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] LINK %x HAS GONE DOWN!\r\n", uId, uId);

      if (uId == 0){  /* 1gbe i/f */
        /* do not set in loopback mode */
#ifndef DO_1GBE_LOOPBACK_TEST
        vDHCPStateMachineReset(pIFObjectPtr); /* this will reset and disable dhcp state machine */
        //uEthernetFabricIPAddress[uId] = 0;        /* TODO: remove global */
        uEthernetGatewayIPAddress[uId] = 0;       /* TODO: remove global */
        //uEthernetSubnet[uId] = 0;                 /* TODO: remove global */
        //pIFObjectPtr->uIFEthernetSubnet = 0;
        IFConfig(pIFObjectPtr, 0, 0);

        /* legacy dhcp states */
        uDHCPState[uId] = DHCP_STATE_IDLE;        /* TODO: remove global */
#endif
      } else {
        /* do not set in loopback mode */
#ifndef DO_40GBE_LOOPBACK_TEST
        vDHCPStateMachineReset(pIFObjectPtr); /* this will reset and disable dhcp state machine */
        //uEthernetFabricIPAddress[uId] = 0;        /* TODO: remove global */
        uEthernetGatewayIPAddress[uId] = 0;       /* TODO: remove global */
        //uEthernetSubnet[uId] = 0;                 /* TODO: remove global */
        //pIFObjectPtr->uIFEthernetSubnet = 0;
        IFConfig(pIFObjectPtr, 0, 0);

        /* legacy dhcp states */
        uDHCPState[uId] = DHCP_STATE_IDLE;        /* TODO: remove global */
        pIFObjectPtr->uIFLinkRxActive = 0;
#endif
      }
    }

    pIFObjectPtr->uIFLinkStatus = LINK_DOWN;
    pIFObjectPtr->uIFEnableArpRequests = ARP_REQUESTS_DISABLE;

    uIGMPSendMessage[uId] = IGMP_DONE_SENDING_MESSAGE;
    /*
     *  Leave IGMP in it's current state but simply stop sending the messages.
     *  This is to combat a link "flap" or switch reboot. We will resend the
     *  igmp messages again when the link / IP is reconfigured.
     */
#if 0
    uIGMPState[uId] = IGMP_STATE_NOT_JOINED;      /* TODO: remove global */
#endif
  }
}

//=================================================================================
//  uRecvPacketFilter
//---------------------------------------------------------------------------------
//  This method applies the packet filtering per layer.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  typePacketFilter
//=================================================================================
typePacketFilter uRecvPacketFilter(struct sIFObject *pIFObjectPtr){
  u16 uL2Type;    /* implemented: Ethernet Frame Type filtering => ARP, IPv4 */
  u8 uL3Type;    /* implemented: IPv4 Protocol Type filtering => ICMP, UDP */

  /* implemented: UDP Datagram Port filtering => DHCP, CONTROL */
  u16 uUDPSrcPort = 0;
  u16 uUDPDstPort = 0;

  u8 uIPHdrLenAdjust = 0;

  typePacketFilter uReturnType = PACKET_FILTER_UNKNOWN;

  u8 *pRxBuffer = NULL;

  SANE_IF(pIFObjectPtr);

  pRxBuffer = pIFObjectPtr->pUserRxBufferPtr;
  if (pRxBuffer == NULL){
    return PACKET_FILTER_ERROR;
  }

  pIFObjectPtr->uRxTotal++;

  /* inspect the ethernet frame type */
  uL2Type = (pRxBuffer[ETH_FRAME_TYPE_OFFSET] << 8) & 0xff00;
  uL2Type = uL2Type | (pRxBuffer[ETH_FRAME_TYPE_OFFSET + 1] & 0xff);
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "layer2 type 0x%04x\r\n",uL2Type);


  switch(uL2Type){
    case ETHER_TYPE_ARP:
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "ARP pkt\r\n");
      pIFObjectPtr->uRxEthArp++;
      uReturnType = PACKET_FILTER_ARP;
      /* no further filtering required */
      break;

    case ETHER_TYPE_IPV4:
      /* further filtering required on ip packet */
      /* inspect the ip header protocol type */
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "IP pkt\r\n");
      pIFObjectPtr->uRxEthIp++;
      uL3Type = pRxBuffer[IP_FRAME_BASE + IP_PROT_OFFSET] & 0xff;
      switch(uL3Type){
        case IPV4_TYPE_ICMP:
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "ICMP pkt\r\n");
          pIFObjectPtr->uRxIpIcmp++;
          uReturnType = PACKET_FILTER_ICMP;
          /* no further filtering required */
          break;

        case IPV4_TYPE_UDP:
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "UDP pkt\r\n");
          pIFObjectPtr->uRxIpUdp++;
          /* further filtering required on udp datagram */
          /* allow for variable ip header lengths */
          /* adjust ip base value if ip length greater than 20 bytes. Min header length = 20 bytes; Max header length = 60 bytes */
          uIPHdrLenAdjust = (((pRxBuffer[IP_FRAME_BASE] & 0x0F) * 4) - 20);
          if (uIPHdrLenAdjust > 40){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "IP header length adjustment of %d seems incorrect!\r\n", uIPHdrLenAdjust);
            return PACKET_FILTER_ERROR;
          }

          /* inspect the udp header port values */
          uUDPSrcPort = (pRxBuffer[uIPHdrLenAdjust + UDP_FRAME_BASE + UDP_SRC_PORT_OFFSET] << 8) & 0xff00;
          uUDPSrcPort = uUDPSrcPort | (pRxBuffer[uIPHdrLenAdjust + UDP_FRAME_BASE + UDP_SRC_PORT_OFFSET + 1] & 0xff);

          uUDPDstPort = (pRxBuffer[uIPHdrLenAdjust + UDP_FRAME_BASE + UDP_DST_PORT_OFFSET] << 8) & 0xff00;
          uUDPDstPort = uUDPDstPort | (pRxBuffer[uIPHdrLenAdjust + UDP_FRAME_BASE + UDP_DST_PORT_OFFSET + 1] & 0xff);

          if (uUDPDstPort == UDP_CONTROL_PORT){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "CTRL pkt\r\n");
            pIFObjectPtr->uRxUdpCtrl++;
            uReturnType = PACKET_FILTER_CONTROL;
          } else if ((uUDPDstPort == BOOTP_CLIENT_PORT) && (uUDPSrcPort == BOOTP_SERVER_PORT)){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "DHCP pkt\r\n");
            pIFObjectPtr->uRxUdpDhcp++;
            uReturnType = PACKET_FILTER_DHCP;
          } else if ((uUDPDstPort == BOOTP_SERVER_PORT) && (uUDPSrcPort == BOOTP_CLIENT_PORT)){
            /* These are probably broadcast DISCOVER / REQUEST packets from other network nodes / SKARABs
               destined for the dhcp server and can probably safely be dropped */
            log_printf(LOG_SELECT_DHCP, LOG_LEVEL_TRACE, "DHCP packet destined for DHCP server received! Can probably safely drop packet!\r\n");
            pIFObjectPtr->uRxDhcpUnknown++;
            pIFObjectPtr->uRxUdpDhcp++;   /* increment total dhcp counter too since this is a dhcp packet (even tho not destined for us) */
            uReturnType = PACKET_FILTER_DROP;
          } else {
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "UNKNOWN UDP ports: [src] 0x%04x & [dst] 0x%04x!\r\n", uUDPSrcPort, uUDPDstPort);
            pIFObjectPtr->uRxUdpUnknown++;
            uReturnType = PACKET_FILTER_UNKNOWN_UDP;
          }
          break;

        default:
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "UNKNOWN IP protocol 0x%02x received!\r\n", uL3Type);
          pIFObjectPtr->uRxIpUnknown++;
          uReturnType = PACKET_FILTER_UNKNOWN_IP;
          break;
      }
      break;

    default:
      /* Handle unimplemented / unknown ethernet frames */
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "UNKNOWN ETH type 0x%04x packet received!\r\n", uL2Type);
      pIFObjectPtr->uRxEthUnknown++;
      uReturnType = PACKET_FILTER_UNKNOWN_ETH;
      break;
  }

  return uReturnType;
}


void IFConfig(struct sIFObject *pIFObjectPtr, u32 ip, u32 mask){
  u8 id;

  SANE_IF(pIFObjectPtr);

  id = pIFObjectPtr->uIFEthernetId;

  pIFObjectPtr->arrIFAddrIP[0] = (ip >> 24) & 0xff;
  pIFObjectPtr->arrIFAddrIP[1] = (ip >> 16) & 0xff;
  pIFObjectPtr->arrIFAddrIP[2] = (ip >>  8) & 0xff;
  pIFObjectPtr->arrIFAddrIP[3] = (ip        & 0xff);

  pIFObjectPtr->arrIFAddrNetmask[0] = (mask >> 24) & 0xff;
  pIFObjectPtr->arrIFAddrNetmask[1] = (mask >> 16) & 0xff;
  pIFObjectPtr->arrIFAddrNetmask[2] = (mask >>  8) & 0xff;
  pIFObjectPtr->arrIFAddrNetmask[3] = (mask        & 0xff);

  pIFObjectPtr->uIFAddrIP = ip;
  pIFObjectPtr->uIFAddrMask = mask;

  /* automatically calculate subnet */
  pIFObjectPtr->uIFEthernetSubnet = (ip & mask);

  /* convert ip to string and cache for later use / printing */
  if (uIPV4_ntoa((char *) (pIFObjectPtr->stringIFAddrIP), ip) != 0){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "I/F  [%02x] Unable to convert IP %x to string.\r\n", id, ip);
  } else {
    pIFObjectPtr->stringIFAddrIP[15] = '\0';
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] Setting IP address to: %s\r\n", id, pIFObjectPtr->stringIFAddrIP);
  }

  /* convert netmask to string and cache for later use / printing */
  if (uIPV4_ntoa((char *) (pIFObjectPtr->stringIFAddrNetmask), mask) != 0){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "I/F  [%02x] Unable to convert Netmask %x to string.\r\n", id, mask);
  } else {
    pIFObjectPtr->stringIFAddrNetmask[15] = '\0';
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] Setting Netmask address to: %s\r\n", id, pIFObjectPtr->stringIFAddrNetmask);
  }
}


void IFCounterIncr(struct sIFObject *pIFObjectPtr, tCounter c){

  SANE_IF(pIFObjectPtr);

  switch (c){
    case RX_TOTAL :
      pIFObjectPtr->uRxTotal++;
      break;
    case RX_ETH_ARP :
      pIFObjectPtr->uRxEthArp++;
      break;
    case RX_ARP_REPLY :
      pIFObjectPtr->uRxArpReply++;
      break;
    case RX_ARP_REQUEST :
      pIFObjectPtr->uRxArpRequest++;
      break;
    case RX_ARP_CONFLICT :
      pIFObjectPtr->uRxArpConflict++;
      break;
    case RX_ARP_INVALID :
      pIFObjectPtr->uRxArpInvalid++;
      break;
    case RX_ETH_IP :
      pIFObjectPtr->uRxEthIp++;
      break;
    case RX_IP_CHK_ERR :
      pIFObjectPtr->uRxIpChecksumErrors++;
      break;
    case RX_IP_ICMP :
      pIFObjectPtr->uRxIpIcmp++;
      break;
    case RX_ICMP_INVALID :
      pIFObjectPtr->uRxIcmpInvalid++;
      break;
    case RX_IP_UDP :
      pIFObjectPtr->uRxIpUdp++;
      break;
    case RX_UDP_CHK_ERR :
      pIFObjectPtr->uRxUdpChecksumErrors++;
      break;
    case RX_UDP_CTRL :
      pIFObjectPtr->uRxUdpCtrl++;
      break;
    case RX_UDP_DHCP :
      pIFObjectPtr->uRxUdpDhcp++;
      break;
    case RX_DHCP_INVALID :
      pIFObjectPtr->uRxDhcpInvalid++;
      break;
    case RX_DHCP_UNKNOWN :
      pIFObjectPtr->uRxDhcpUnknown++;
      break;
    case RX_UDP_UNKNOWN :
      pIFObjectPtr->uRxUdpUnknown++;
      break;
    case RX_IP_UNKNOWN :
      pIFObjectPtr->uRxIpUnknown++;
      break;
    case RX_ETH_UNKNOWN :
      pIFObjectPtr->uRxEthUnknown++;
      break;
    case TX_TOTAL :
      pIFObjectPtr->uTxTotal++;
      break;
    case TX_ETH_ARP_REQ_OK :
      pIFObjectPtr->uTxEthArpRequestOk++;
      break;
    case TX_ETH_ARP_REPLY_OK :
      pIFObjectPtr->uTxEthArpReplyOk++;
      break;
    case TX_ETH_ARP_ERR :
      pIFObjectPtr->uTxEthArpErr++;
      break;
    case TX_ETH_LLDP_OK :
      pIFObjectPtr->uTxEthLldpOk++;
      break;
    case TX_ETH_LLDP_ERR :
      pIFObjectPtr->uTxEthLldpErr++;
      break;
    case TX_IP_ICMP_REPLY_OK :
      pIFObjectPtr->uTxIpIcmpReplyOk++;
      break;
    case TX_IP_ICMP_REPLY_ERR :
      pIFObjectPtr->uTxIpIcmpReplyErr++;
      break;
    case TX_IP_IGMP_OK :
      pIFObjectPtr->uTxIpIgmpOk++;
      break;
    case TX_IP_IGMP_ERR :
      pIFObjectPtr->uTxIpIgmpErr++;
      break;
    case TX_UDP_DHCP_OK :
      pIFObjectPtr->uTxUdpDhcpOk++;
      break;
    case TX_UDP_DHCP_ERR :
      pIFObjectPtr->uTxUdpDhcpErr++;
      break;
    case TX_UDP_CTRL_OK :
      pIFObjectPtr->uTxUdpCtrlOk++;
      break;
    case TX_UDP_CTRL_ACK :
      pIFObjectPtr->uTxUdpCtrlAck++;
      break;
    case TX_UDP_CTRL_NACK :
      pIFObjectPtr->uTxUdpCtrlNack++;
      break;
    default:
      break;
  }
}

/* hide the interface state handles */
struct sIFObject *lookup_if_handle_by_id(u8 id){
  /*
   * statically initialise the pool of state handles depending on the
   * number of interfaces requested at compile time
   */
  static struct sIFObject IFContext[NUM_ETHERNET_INTERFACES];

  SANE_IF_ID(id);

  return &IFContext[id];
}
