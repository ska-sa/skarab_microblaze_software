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

#include "print.h"
#include "diagnostics.h"

/*-------- Network diagnostics --------*/

//=================================================================================
//  PrintInterfaceCounters
//---------------------------------------------------------------------------------
//  Print all the counters for a specific interface to the terminal screen.
//
//  Parameter	      Dir   Description
//  ---------	      ---	  -----------
//  pIFObjectPtr    IN    handle to IF state object
//
//  Return
//  ------
//  none
//=================================================================================
void PrintInterfaceCounters(struct sIFObject *pIFObj){
  if (NULL == pIFObj){
    return;
  }

  pIFObj->uTxTotal = pIFObj->uTxEthArpReplyOk + pIFObj->uTxEthArpRequestOk + pIFObj->uTxEthLldpOk +
                     pIFObj->uTxIpIcmpReplyOk + pIFObj->uTxIpIgmpOk + pIFObj->uTxUdpDhcpOk + pIFObj->uTxUdpCtrlOk;

  debug_printf("IF [%d]:  STATUS: %s  IP: %s  Netmask: %s\r\n", pIFObj->uIFEthernetId,
                                                                pIFObj->uIFLinkStatus == LINK_UP ? "UP" : "DOWN",
                                                                pIFObj->stringIFAddrIP,
                                                                pIFObj->stringIFAddrNetmask);
  debug_printf(" Rx%1s%-13s%11d%2s", "","Total:",         pIFObj->uRxTotal,           "|");
  debug_printf(" Tx%1s%-13s%11d%2s", "","Total:",         pIFObj->uTxTotal,           "\r\n");
  debug_printf(" Rx%2s%-12s%11d%2s", "", "ETH Unknown:",  pIFObj->uRxEthUnknown,      "|");
  debug_printf(" Tx%2sARP\r\n", "");
  debug_printf(" Rx%2s%-12s%11d%2s", "", "ARP:",          pIFObj->uRxEthArp,          "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uTxEthArpReplyOk,   "\r\n");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uRxArpReply,        "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Request:",      pIFObj->uTxEthArpRequestOk, "\r\n");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Request:",      pIFObj->uRxArpRequest,      "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxEthArpErr,       "\r\n");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Conflict:",     pIFObj->uRxArpConflict,     "|");
  debug_printf(" Tx%2sLLDP\r\n", "");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Invalid:",      pIFObj->uRxArpInvalid,      "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxEthLldpOk,       "\r\n");
  debug_printf(" Rx%2s%-12s%11d%2s", "", "IP:",           pIFObj->uRxEthIp,           "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxEthLldpErr,      "\r\n");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Chksm Err:",    pIFObj->uRxIpChecksumErrors,"|");
  debug_printf(" Tx%2sICMP\r\n", "");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "Unknown:",      pIFObj->uRxIpUnknown,       "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Reply:",        pIFObj->uTxIpIcmpReplyOk,   "\r\n");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "ICMP:",         pIFObj->uRxIpIcmp,          "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxIpIcmpReplyErr,  "\r\n");
  debug_printf(" Rx%4s%-10s%11d%2s", "", "Invalid:",      pIFObj->uRxIcmpInvalid,     "|");
  debug_printf(" Tx%2sIGMP\r\n", "");
  debug_printf(" Rx%3s%-11s%11d%2s", "", "UDP:",          pIFObj->uRxIpUdp,           "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxIpIgmpOk,        "\r\n");
  debug_printf(" Rx%4s%-10s%11d%2s", "", "Unknown:",      pIFObj->uRxUdpUnknown,      "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxIpIgmpErr,       "\r\n");
  debug_printf(" Rx%4s%-10s%11d%2s", "", "CTRL:",         pIFObj->uRxUdpCtrl,         "|");
  debug_printf(" Tx%2sDHCP\r\n", "");
  debug_printf(" Rx%4s%-10s%11d%2s", "", "DHCP:",         pIFObj->uRxUdpDhcp,         "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxUdpDhcpOk,       "\r\n");
  debug_printf(" Rx%5s%-9s%11d%2s", "", "Invalid:",      pIFObj->uRxDhcpInvalid,     "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Err:",          pIFObj->uTxUdpDhcpErr,      "\r\n");
  debug_printf("%30s", "|");
  debug_printf(" Tx%2sCTRL\r\n", "");
  debug_printf("%30s", "|");
  debug_printf(" Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxUdpCtrlOk,       "\r\n");
}
