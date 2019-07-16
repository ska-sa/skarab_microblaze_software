/**----------------------------------------------------------------------------
 *   FILE:       diagnostics.c
 *   BRIEF:      Implementation of various diagnostic functions
 *
 *   DATE:       MARCH 2018
 *
 *   COMPANY:    SKA SA
 *   AUTHOR:     R van Wyk
 *
 *------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

#include "constant_defs.h"
#include "logging.h"
#include "diagnostics.h"

/*-------- Network diagnostics --------*/

//=================================================================================
//  PrintInterfaceCounters
//---------------------------------------------------------------------------------
//  Print all the counters for a specific interface to the terminal screen.
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  none
//=================================================================================
void PrintInterfaceCounters(struct sIFObject *pIFObj){
  int i;

  if (NULL == pIFObj){
    return;
  }

  pIFObj->uTxTotal = pIFObj->uTxEthArpReplyOk + pIFObj->uTxEthArpRequestOk + pIFObj->uTxEthLldpOk +
    pIFObj->uTxIpIcmpReplyOk + pIFObj->uTxIpIgmpOk + pIFObj->uTxUdpDhcpOk + pIFObj->uTxUdpCtrlOk;

  for (i = 0; i < 60; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "-");
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "IF [%d]:  STATUS: %s  IP: %s  Netmask: %s\r\n", pIFObj->uIFEthernetId,
      pIFObj->uIFLinkStatus == LINK_UP ? "UP" : "DOWN",
      pIFObj->stringIFAddrIP,
      pIFObj->stringIFAddrNetmask);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%1s%-13s%11d%2s", "","Total:",         pIFObj->uRxTotal,           "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%1s%-13s%11d%2s", "","Total:",         pIFObj->uTxTotal,           "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%2s%-12s%11d%2s", "", "ETH Unknown:",  pIFObj->uRxEthUnknown,      "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sARP\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%2s%-12s%11d%2s", "", "ARP:",          pIFObj->uRxEthArp,          "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uTxEthArpReplyOk,   "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uRxArpReply,        "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Request:",      pIFObj->uTxEthArpRequestOk, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Request:",      pIFObj->uRxArpRequest,      "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxEthArpErr,       "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Conflict:",     pIFObj->uRxArpConflict,     "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sLLDP\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Invalid:",      pIFObj->uRxArpInvalid,      "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxEthLldpOk,       "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%2s%-12s%11d%2s", "", "IP:",           pIFObj->uRxEthIp,           "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxEthLldpErr,      "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Chksm Err:",    pIFObj->uRxIpChecksumErrors,"|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sICMP\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Unknown:",      pIFObj->uRxIpUnknown,       "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uTxIpIcmpReplyOk,   "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "ICMP:",         pIFObj->uRxIpIcmp,          "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxIpIcmpReplyErr,  "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "Invalid:",      pIFObj->uRxIcmpInvalid,     "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sIGMP\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "UDP:",          pIFObj->uRxIpUdp,           "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxIpIgmpOk,        "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "Unknown:",      pIFObj->uRxUdpUnknown,      "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxIpIgmpErr,       "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "CTRL:",         pIFObj->uRxUdpCtrl,         "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sDHCP\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "DHCP:",         pIFObj->uRxUdpDhcp,         "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxUdpDhcpOk,       "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%5s%-9s%11d%2s", "", "Invalid:",       pIFObj->uRxDhcpInvalid,     "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxUdpDhcpErr,      "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%5s%-9s%11d%2s", "", "Unknown:",       pIFObj->uRxDhcpUnknown,     "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%2sCTRL\r\n", "");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "%30s", "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxUdpCtrlOk,       "\r\n");

  for (i = 0; i < 60; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "-");
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
}
