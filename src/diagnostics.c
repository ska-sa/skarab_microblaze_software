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
#include "id.h"
#include "register.h"

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
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " RX%3s%-11s%11d%2s", "", "IGMP:",         pIFObj->uRxIpIgmp,          "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Tx%3s%-11s%11d%2s", "", "Ok:",           pIFObj->uTxUdpCtrlOk,       "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "Dropped:",      pIFObj->uRxIpIgmpDropped,   "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " RX%3s%-11s%11d%2s", "", "PIM:",          pIFObj->uRxIpPim,           "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "Dropped:",      pIFObj->uRxIpPimDropped,    "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " RX%3s%-11s%11d%2s", "", "TCP:",          pIFObj->uRxIpTcp,           "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%4s%-10s%11d%2s", "", "Dropped:",      pIFObj->uRxIpTcpDropped,    "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%2s%-12s%11d%2s", "", "LLDP:",         pIFObj->uRxEthLldp,         "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " Rx%3s%-11s%11d%2s", "", "Dropped:",      pIFObj->uRxEthLldpDropped,  "|");
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");

  for (i = 0; i < 60; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "-");
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
}

static const char * const version = VENDOR_ID;

void PrintVersionInfo(){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Embedded software version: %d.%d.%d\r\n",
      EMBEDDED_SOFTWARE_VERSION_MAJOR,
      EMBEDDED_SOFTWARE_VERSION_MINOR,
      EMBEDDED_SOFTWARE_VERSION_PATCH);
  log_printf(LOG_SELECT_ALL, LOG_LEVEL_ALWAYS, "Running ELF version: %s\r\n", version);
}

const char *GetVersionInfo(){
  return version;
}


void ReadAndPrintPeralexSerial(){
  u8 status;
  u8 uPxSerial[ID_PX_SERIAL_LEN];

  status = get_peralex_serial(uPxSerial, ID_PX_SERIAL_LEN);
  if (status == XST_FAILURE){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Failed to read Peralex serial number.\r\n");
  }
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] Motherboard Px Serial Number: x%02x%02x%02x d%02d%02d%02d\r\n",
      uPxSerial[0], uPxSerial[1], uPxSerial[2],uPxSerial[0], uPxSerial[1], uPxSerial[2]);
}


/*  This method reads and prints out the FPGA DNA value */
void ReadAndPrintFPGADNA()
{
  u32 uDNALow = ReadBoardRegister(C_RD_FPGA_DNA_LOW_ADDR);
  u32 uDNAHigh = ReadBoardRegister(C_RD_FPGA_DNA_HIGH_ADDR);
  u8 uBitShift = 28;
  u8 uNibbleCount;
  u32 uNibble;

  // Print out each nibble in hex format
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "FPGA [..] DNA: 0x");

  for (uNibbleCount = 0; uNibbleCount < 8; uNibbleCount++)
  {
    uNibble = (uDNAHigh >> uBitShift) & 0xF;
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "%x", uNibble);
    if (uBitShift > 0)
      uBitShift = uBitShift - 4;
  }

  uBitShift = 28;

  for (uNibbleCount = 0; uNibbleCount < 8; uNibbleCount++)
  {
    uNibble = (uDNALow >> uBitShift) & 0xF;
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "%x", uNibble);
    if (uBitShift > 0)
      uBitShift = uBitShift - 4;
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "\r\n");
}


#ifndef PRUNE_CODEBASE_DIAGNOSTICS
/*
 * print the interface object data structure
 */
void print_interface_internals(u8 physical_interface_id){
  int i;
  struct sIFObject *ifptr;

  ifptr = lookup_if_handle_by_id(physical_interface_id);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFMagic x%08x\r\n", ifptr->uIFMagic);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "pUserTxBufferPtr x%p\r\n", ifptr->pUserTxBufferPtr);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uUserTxBufferSize u%u\r\n", ifptr->uUserTxBufferSize);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uMsgSize u%u\r\n", ifptr->uMsgSize );
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "pUserRxBufferPtr x%p\r\n", ifptr->pUserRxBufferPtr);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uUserRxBufferSize u%u\r\n", ifptr->uUserRxBufferSize);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uNumWordsRead u%u\r\n", ifptr->uNumWordsRead);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFLinkStatus u%u\r\n", ifptr->uIFLinkStatus);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFLinkRxActive u%u\r\n", ifptr->uIFLinkRxActive);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "arrIFAddrMac[%d]", IF_MAC_ARR_LEN);
  for (i = 0; i < IF_MAC_ARR_LEN; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " %02x", ifptr->arrIFAddrMac[i])
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");

  /* currently not being setso don't display - hostname is set in the dhcp API functions from main() */
  //log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "stringHostname[%d] %s\r\n", IF_HOSTNAME_STR_LEN, ifptr->stringHostname);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "arrIFAddrIP[%d]", IF_IPADDR_ARR_LEN);
  for (i = 0; i < IF_IPADDR_ARR_LEN; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " %02x", ifptr->arrIFAddrIP[i])
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "stringIFAddrIP[%d] %s\r\n", IF_IPADDR_STR_LEN, ifptr->stringIFAddrIP);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFAddrIP x%08x\r\n", ifptr->uIFAddrIP);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "arrIFAddrNetmask[%d]", IF_NETMASK_ARR_LEN);
  for (i = 0; i < IF_NETMASK_ARR_LEN; i++){
    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, " %02x", ifptr->arrIFAddrNetmask[i])
  }
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "stringIFAddrNetmask[%d] %s\r\n", IF_NETMASK_STR_LEN, ifptr->stringIFAddrNetmask);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFAddrMask x%08x\r\n", ifptr->uIFAddrMask);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFEthernetId u%u\r\n", ifptr->uIFEthernetId);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFEthernetSubnet x%08x\r\n", ifptr->uIFEthernetSubnet);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFEnableArpRequests u%u\r\n", ifptr->uIFEnableArpRequests);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFCurrentArpRequest u%u\r\n", ifptr->uIFCurrentArpRequest);
  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "uIFValidPacketRx u%u\r\n", ifptr->uIFValidPacketRx);

  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "\r\n");
}



/*
 * print the dhcp object data structure
 */
void print_dhcp_internals(u8 physical_interface_id){
  int i;
  struct sDHCPObject *dhcpptr;
  struct sIFObject *ifptr;

  static const char *dhcp_state_string_lookup[] = {
    [INIT] = "INIT",
    [RANDOMIZE] = "RANDOMIZE",
    [SELECT] = "SELECT",
    [WAIT] = "WAIT",
    [REQUEST] = "REQUEST",
    [BOUND] = "BOUND",
    [RENEW] = "RENEW",
    [REBIND] = "REBIND"
  };

  ifptr = lookup_if_handle_by_id(physical_interface_id);
  dhcpptr = &(ifptr->DHCPContextState);

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "physical interface id u%u\r\n", physical_interface_id);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPMagic x%08x\r\n", dhcpptr->uDHCPMagic);

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPMessageReady u%u\r\n", dhcpptr->uDHCPMessageReady);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPTx u%u\r\n", dhcpptr->uDHCPTx);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRx u%u\r\n", dhcpptr->uDHCPRx);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPErrors u%u\r\n", dhcpptr->uDHCPErrors);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPInvalid u%u\r\n", dhcpptr->uDHCPInvalid);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRetries u%u\r\n", dhcpptr->uDHCPRetries);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPTimeoutStatus u%u\r\n", dhcpptr->uDHCPTimeoutStatus);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRebootReqs u%u\r\n", dhcpptr->uDHCPRebootReqs);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPTimeout u%u\r\n", dhcpptr->uDHCPTimeout);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "tDHCPCurrentState %s\r\n", dhcp_state_string_lookup[dhcpptr->tDHCPCurrentState] );
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPInternalTimer u%u\r\n", dhcpptr->uDHCPInternalTimer);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPCurrentClkTick u%u\r\n", dhcpptr->uDHCPCurrentClkTick);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPCachedClkTick u%u\r\n", dhcpptr->uDHCPCachedClkTick);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPExternalTimerTick u%u\r\n", dhcpptr->uDHCPExternalTimerTick);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRandomWait u%u\r\n", dhcpptr->uDHCPRandomWait);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRandomWaitCached u%u\r\n", dhcpptr->uDHCPRandomWaitCached);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPSMRetryInterval u%u\r\n", dhcpptr->uDHCPSMRetryInterval);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPSMInitWait u%u\r\n", dhcpptr->uDHCPSMInitWait);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPT1 u%u\r\n", dhcpptr->uDHCPT1);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPT2 u%u\r\n", dhcpptr->uDHCPT2);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPLeaseTime u%u\r\n", dhcpptr->uDHCPLeaseTime);

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "arrDHCPNextHopMacCached[%d]", DHCP_MAC_ARR_LEN);
  for (i = 0; i < DHCP_MAC_ARR_LEN; i++){
    log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, " %02x", dhcpptr->arrDHCPNextHopMacCached[i])
  }
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "\r\n");

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "arrDHCPAddrServerCached[%d]", DHCP_IPADDR_ARR_LEN);
  for (i = 0; i < DHCP_IPADDR_ARR_LEN; i++){
    log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, " %02x", dhcpptr->arrDHCPAddrServerCached[i])
  }
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "\r\n");

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPXidCached x%08x\r\n", dhcpptr->uDHCPXidCached);

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "arrDHCPHostName %s\r\n", dhcpptr->arrDHCPHostName);

  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPRegisterFlags x%02x\r\n", dhcpptr->uDHCPRegisterFlags);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "uDHCPInitUnboundTimer u%u\r\n", dhcpptr->uDHCPInitUnboundTimer);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "poll interval %u ms\r\n", POLL_INTERVAL);
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "\r\n");
}

#endif
