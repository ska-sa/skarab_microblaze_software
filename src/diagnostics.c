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
