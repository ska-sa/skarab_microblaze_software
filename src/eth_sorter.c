/**------------------------------------------------------------------------------
 *  FILE NAME            : eth_sorter.c
 * ------------------------------------------------------------------------------
 *  COMPANY              : PERALEX ELECTRONIC (PTY) LTD
 * ------------------------------------------------------------------------------
 *  COPYRIGHT NOTICE :
 *
 *  The copyright, manufacturing and patent rights stemming from this
 *  document in any form are vested in PERALEX ELECTRONICS (PTY) LTD.
 *
 *  (c) Peralex 2011
 *
 *  PERALEX ELECTRONICS (PTY) LTD has ceded these rights to its clients
 *  where contractually agreed.
 * ------------------------------------------------------------------------------
 *  DESCRIPTION :
 *
 *  This file contains the implementation of functions to sort Ethernet packets.
 * ------------------------------------------------------------------------------*/

#include <xstatus.h>
#include <xil_types.h>
#include <xil_io.h>
#include <xparameters.h>

#include "eth_sorter.h"
#include "constant_defs.h"
#include "custom_constants.h"
#include "eth_mac.h"
#include "sensors.h"
#include "improved_read_write.h"
#include "invalid_nack.h"
#include "flash_sdram_controller.h"
#include "i2c_master.h"
#include "isp_spi_controller.h"
#include "register.h"
#include "delay.h"
#include "net_utils.h"
#include "dhcp.h"
#include "one_wire.h"
#include "logging.h"
#include "qsfp.h"
#include "fault_log.h"
#include "igmp.h"
#include "time.h"
#include "fanctrl.h"
#include "error.h"

//=================================================================================
//  CalculateIPChecksum
//--------------------------------------------------------------------------------
//  This method calculates the IPv4 header checksum.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uChecksum IN    Initial checksum value
//  uLength   IN    Number of bytes
//  pHeaderPtr  IN    Packet header pointer
//
//  Return
//  ------
//  Checksum
//=================================================================================
u32 CalculateIPChecksum(u32 uChecksum, u32 uLength, u16 *pHeaderPtr)
{
  u32 k;
  u32 l;

  //l = uLength - (uLength & 0x01u);
  l = uLength;

  for (k = 0; k != l; ++k)
    uChecksum += (u32) *pHeaderPtr++;

  //uChecksum = (uChecksum & 0x0FFFFu) + (uChecksum >> 16);

  //if ((uLength & 0x01u) != 0)
  //  uChecksum += (u32) pHeaderPtr[1];

  //uChecksum = (uChecksum & 0x0FFFFu) + (uChecksum >> 16);
  //uChecksum = (uChecksum & 0x0FFFFu) + (uChecksum >> 16);

  while (uChecksum>>16)
  {
    uChecksum = (uChecksum & 0xFFFF)+(uChecksum >> 16);
  }

  return uChecksum;
}

//=================================================================================
//  CheckIPV4Header
//--------------------------------------------------------------------------------
//  This method validates the IPv4 packet header.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uIPAddress      IN  IP address of FPGA fabric
//  uSubnet       IN  Subnet mask
//  uPacketLength   IN  Packet length
//  pIPHeaderPointer  IN  Pointer to header
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int CheckIPV4Header(u32 uIPAddress, u32 uSubnet, u32 uPacketLength, u8 * pIPHeaderPointer)
{
  u32 uIPHeaderLength;
  u32 uChecksum;
  u8 uIPAddressMatch;
  u8 uIPBroadcast;
  u8 uSubnetBroadcast;
  u32 uDestinationIPAddress;
  /* u16 uIndex; */

  if (uPacketLength < sizeof(struct sIPV4Header))
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IP Length problem\r\n");
    return XST_FAILURE;
  }

  struct sIPV4Header *IPHeader = (struct sIPV4Header *) pIPHeaderPointer;

  uDestinationIPAddress = (IPHeader->uDestinationIPHigh << 16) | IPHeader->uDestinationIPLow;

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "VERS: %x TOS: %x LEN: %x ID: %x FLGS: %x TTL: %x PROT: %x CHK: %x SRC: %x %x DEST: %x %x\r\n", IPHeader->uVersion, IPHeader->uTypeOfService, IPHeader->uTotalLength, IPHeader->uIdentification, IPHeader->uFlagsFragment, IPHeader->uTimeToLive, IPHeader->uProtocol, IPHeader->uChecksum,  IPHeader->uSourceIPHigh, IPHeader->uSourceIPLow, IPHeader->uDestinationIPHigh, IPHeader->uDestinationIPLow );

  if (uPacketLength < IPHeader->uTotalLength)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IP Total length problem PKT: %x\r\n", uPacketLength);

    //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "VERS: %x TOS: %x LEN: %x ID: %x FLGS: %x TTL: %x PROT: %x CHK: %x SRC: %x %x DEST: %x %x\r\n", IPHeader->uVersion, IPHeader->uTypeOfService, IPHeader->uTotalLength, IPHeader->uIdentification, IPHeader->uFlagsFragment, IPHeader->uTimeToLive, IPHeader->uProtocol, IPHeader->uChecksum,  IPHeader->uSourceIPHigh, IPHeader->uSourceIPLow, IPHeader->uDestinationIPHigh, IPHeader->uDestinationIPLow );

    /*  TODO FIXME uReceiveBuffer now a 2D array
        for (uIndex = 0; uIndex < 256; uIndex++)
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "%x DATA: %x\r\n", uIndex, uReceiveBuffer[uIndex]);
     */

    return XST_FAILURE;
  }

  // IP version
  if ((IPHeader->uVersion & 0xF0) != 0x40)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IP Version problem\r\n");
    return XST_FAILURE;
  }

  // IP checksum - long version
  uIPHeaderLength = 4 * (IPHeader->uVersion & 0x0F);
  uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) pIPHeaderPointer);

  if (uChecksum != 0xFFFFu)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IP Checksum problem\r\n");
    return XST_FAILURE;
  }

  // IP address

  if (uIPAddress == uDestinationIPAddress)
    uIPAddressMatch = 1;
  else
    uIPAddressMatch = 0;

  if (uDestinationIPAddress == 0xFFFFFFFF)
    uIPBroadcast = 1;
  else
    uIPBroadcast = 0;

  if ((((uDestinationIPAddress ^ uIPAddress) & uSubnet) == 0)&&((uDestinationIPAddress | uSubnet) == 0xFFFFFFFFu))
    uSubnetBroadcast = 1;
  else
    uSubnetBroadcast = 0;

  if (! (uIPAddressMatch || uIPBroadcast || uSubnetBroadcast))
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  ExtractIPV4FieldsAndGetPayloadPointer
//--------------------------------------------------------------------------------
//  This method extracts the useful information from the IPV4 header and returns
//  a pointer to the start of the IP payload.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pIPHeaderPointer  IN  Pointer to IP header
//  uIPPayloadLength  OUT Length of IP payload
//  uResponseIPAddr   OUT Source IP address (use as destination in response)
//  uProtocol     OUT IP packet protocol
//  uTOS        OUT IP Type Of Service
//
//  Return
//  ------
//  Pointer to IP payload
//=================================================================================
u8 * ExtractIPV4FieldsAndGetPayloadPointer(u8 *pIPHeaderPointer, u32 *uIPPayloadLength, u32 *uResponseIPAddr, u32 *uProtocol, u32 * uTOS)
{
  struct sIPV4Header *IPHeader = (struct sIPV4Header *) pIPHeaderPointer;
  u32 uIPHeaderLength = 4 * (IPHeader->uVersion & 0x0F);

  *uIPPayloadLength = IPHeader->uTotalLength - uIPHeaderLength;
  *uResponseIPAddr = (IPHeader->uSourceIPHigh << 16) | IPHeader->uSourceIPLow;
  *uProtocol = IPHeader->uProtocol;
  *uTOS = IPHeader->uTypeOfService;

  return (pIPHeaderPointer + uIPHeaderLength);
}

//=================================================================================
//  ExtractIPV4FieldsAndGetUDPPointer
//--------------------------------------------------------------------------------
//  This method extracts the useful information from the IPV4 header and returns
//  a pointer to the start of the IP payload.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pIPHeaderPointer  IN    Pointer to IP header
//  uIPPayloadLength  IN    Length of IP payload
//  pUdpHeaderPointer IN    Pointer to UDP header
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int CheckUdpHeader(u8 *pIPHeaderPointer, u32 uIPPayloadLength, u8 *pUdpHeaderPointer)
{
  u32 uIPHeaderLength;
  u32 uChecksum;
  u32 uUdpLength;

  if (uIPPayloadLength < sizeof(struct sUDPHeader))
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "UDP Length problem\r\n");
    return XST_FAILURE;
  }

  struct sUDPHeader *UDPHeader = (struct sUDPHeader *) pUdpHeaderPointer;
  if (uIPPayloadLength < UDPHeader->uTotalLength)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "UDP Total length problem\r\n");
    return XST_FAILURE;
  }

  // UDP pseudo header bits
  struct sIPV4Header *IPHeader = (struct sIPV4Header *) pIPHeaderPointer;
  uIPHeaderLength = 4 * (IPHeader->uVersion & 0x0F);
  uChecksum = (IPHeader->uProtocol & 0x0FFu) + ((IPHeader->uTotalLength - uIPHeaderLength) & 0x0FFFFu);

  uChecksum += IPHeader->uSourceIPHigh + IPHeader->uSourceIPLow;
  uChecksum += IPHeader->uDestinationIPHigh + IPHeader->uDestinationIPLow;

  uUdpLength = UDPHeader->uTotalLength;
  uChecksum = CalculateIPChecksum(uChecksum, (uUdpLength / 2), (u16 *) pUdpHeaderPointer);

  if (uChecksum != 0xFFFFu)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "UDP Checksum problem\r\n");
    return XST_FAILURE;
  }

  return XST_SUCCESS;
}

//=================================================================================
//  ExtractUdpFieldsAndGetPayloadPointer
//--------------------------------------------------------------------------------
//  This method extracts the useful information from the UDP header and returns
//  a pointer to the start of the UDP payload.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pUdpHeaderPointer IN  Pointer to UDP header
//  uPayloadLength    OUT Length of UDP payload
//  uSourcePort     OUT Source port address
//  uDestinationPort  OUT Destination port address
//
//  Return
//  ------
//  Pointer to UDP payload
//=================================================================================
u8 *ExtractUdpFieldsAndGetPayloadPointer(u8 *pUdpHeaderPointer,u32 *uPayloadLength, u32 *uSourcePort, u32 *uDestinationPort)
{
  struct sUDPHeader *UdpHeader = (struct sUDPHeader *) pUdpHeaderPointer;

  *uPayloadLength = UdpHeader->uTotalLength - sizeof(struct sUDPHeader);
  *uSourcePort = UdpHeader->uSourcePort;
  *uDestinationPort = UdpHeader->uDestinationPort;

  return (pUdpHeaderPointer + sizeof(struct sUDPHeader));
}

/* Forward declarations */
static int GetCurrentLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int GetVoltageLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int GetFanControllerLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 *uResponseLength);
static int ClearFanControllerLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int ResetDHCPStateMachine(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int MulticastLeaveGroup(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int GetDHCPMonitorTimeout(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int GetMicroblazeUptime(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int FPGAFanControllerUpdateHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
static int GetFPGAFanControllerLUTHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

//=================================================================================
//  CommandSorter
//--------------------------------------------------------------------------------
//  This method calls the difference command handlers depending on command received.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN    ID of the interface the command was received on
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Location to put response packet
//  uResponseLength     OUT Length of response packet payload
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int CommandSorter(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  int iStatus;
  sCommandHeaderT *Command = (sCommandHeaderT *) pCommand;

  iStatus = CheckCommandPacket(pCommand, uCommandLength);

  if (iStatus == XST_SUCCESS)
  {
    if (Command->uCommandType == WRITE_REG)
      return(WriteRegCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == READ_REG)
      return(ReadRegCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == WRITE_WISHBONE)
      return(WriteWishboneCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == READ_WISHBONE)
      return(ReadWishboneCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == WRITE_I2C)
      return(WriteI2CCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == READ_I2C)
      return(ReadI2CCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == SDRAM_RECONFIGURE)
      return(SdramReconfigureCommandHandler(uId, pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == READ_FLASH_WORDS)
      return(ReadFlashWordsCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == PROGRAM_FLASH_WORDS)
      return(ProgramFlashWordsCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == ERASE_FLASH_BLOCK)
      return(EraseFlashBlockCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == READ_SPI_PAGE)
      return(ReadSpiPageCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == PROGRAM_SPI_PAGE)
      return(ProgramSpiPageCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == ERASE_SPI_SECTOR)
      return(EraseSpiSectorCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == ONE_WIRE_READ_ROM_CMD)
      return(OneWireReadRomCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == ONE_WIRE_DS2433_WRITE_MEM)
      return(OneWireDS2433WriteMemCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == ONE_WIRE_DS2433_READ_MEM)
      return(OneWireDS2433ReadMemCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == DEBUG_CONFIGURE_ETHERNET)
      return(DebugConfigureEthernetCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == DEBUG_ADD_ARP_CACHE_ENTRY)
      return(DebugAddARPCacheEntryCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_EMBEDDED_SOFTWARE_VERS)
      return(GetEmbeddedSoftwareVersionCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == PMBUS_READ_I2C)
      return(PMBusReadI2CBytesCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == CONFIGURE_MULTICAST)
      return(ConfigureMulticastCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == DEBUG_LOOPBACK_TEST)
      return(DebugLoopbackTestCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == QSFP_RESET_AND_PROG)
      return(QSFPResetAndProgramCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == HMC_READ_I2C)
      return(HMCReadI2CBytesCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == HMC_WRITE_I2C)
      return(HMCWriteI2CBytesCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_SENSOR_DATA)
      return(GetSensorDataHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == SET_FAN_SPEED)
      return(SetFanSpeedHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == BIG_READ_WISHBONE)
      return(BigReadWishboneCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == BIG_WRITE_WISHBONE)
      return(BigWriteWishboneCommandHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == SDRAM_PROGRAM_OVER_WISHBONE)
      return(SDRAMProgramOverWishboneCommandHandler(uId, pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == SET_DHCP_TUNING_DEBUG)
      return(SetDHCPTuningDebugCommandHandler(uId, pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_DHCP_TUNING_DEBUG)
      return(GetDHCPTuningDebugCommandHandler(uId, pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_CURRENT_LOGS)
      return(GetCurrentLogsHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_VOLTAGE_LOGS)
      return(GetVoltageLogsHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_FANCONTROLLER_LOGS)
      return(GetFanControllerLogsHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == CLEAR_FANCONTROLLER_LOGS)
      return(ClearFanControllerLogsHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == DHCP_RESET_STATE_MACHINE)
      return(ResetDHCPStateMachine(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == MULTICAST_LEAVE_GROUP)
      return(MulticastLeaveGroup(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_DHCP_MONITOR_TIMEOUT)
      return(GetDHCPMonitorTimeout(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_MICROBLAZE_UPTIME)
      return(GetMicroblazeUptime(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == FPGA_FANCONTROLLER_UPDATE)
      return(FPGAFanControllerUpdateHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else if (Command->uCommandType == GET_FPGA_FANCONTROLLER_LUT)
      return(GetFPGAFanControllerLUTHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
    else{
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Invalid Opcode Detected!\r\n");
      return(InvalidOpcodeHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
      //return XST_FAILURE;
    }

  }
  else
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Invalid Opcode Detected: Out of Range!\r\n");
  return(InvalidOpcodeHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
  //return XST_FAILURE;

  return XST_SUCCESS;

}

//=================================================================================
//  CheckCommandPacket
//--------------------------------------------------------------------------------
//  This method does some basic checks on the command received.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand    IN  Pointer to command header
//  uCommandLength  IN  Length of command
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int CheckCommandPacket(u8 * pCommand, u32 uCommandLength)
{
  //static u16 uPreviousSequenceNumber = 0;
  sCommandHeaderT *Command = (sCommandHeaderT *) pCommand;

  if (uCommandLength < sizeof(sCommandHeaderT))
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Length problem!\r\n");
    return XST_FAILURE;
  }

  // Command must be odd (response even)
  if ((Command->uCommandType & 1) != 1)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Command type problem!\r\n");
    return XST_FAILURE;
  }

  // Out of range
  if (Command->uCommandType > HIGHEST_DEFINED_COMMAND)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Command range problem!\r\n");
    return XST_FAILURE;
  }

  // Sequence number should not be same as previous sequence number
  //if (Command->uSequenceNumber == uPreviousSequenceNumber)
  //{
  //  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Sequence number problem!\r\n");
  //  return XST_FAILURE;
  //}

  // Update sequence number
  //uPreviousSequenceNumber = Command->uSequenceNumber;

  return XST_SUCCESS;
}

//=================================================================================
//  CreateResponsePacket
//--------------------------------------------------------------------------------
//  This method creates the Ethernet, IP and UDP headers for the response packet.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId         IN  ID of selected Ethernet interface
//  uResponsePacketPtr  OUT Pointer to construct response packet
//  uResponseLength   IN  Length of response payload
//
//  Return
//  ------
//  None
//=================================================================================
void CreateResponsePacket(u8 uId, u8 * uResponsePacketPtr, u32 uResponseLength)
{
  struct sIFObject *pIF;
  struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) uResponsePacketPtr;
  u8 * uIPResponseHeader = uResponsePacketPtr + sizeof(sEthernetHeaderT);
  struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPResponseHeader;
  u8 * uUdpResponseHeader = uIPResponseHeader + sizeof(sIPV4HeaderT);
  struct sUDPHeader * UdpHeader = (struct sUDPHeader *) uUdpResponseHeader;
  u32 uChecksum;
  u32 uIPHeaderLength = sizeof(sIPV4HeaderT);
  u32 uUdpLength = uResponseLength + sizeof(sUDPHeaderT);

  pIF = lookup_if_handle_by_id(uId);

  EthernetHeader->uDestMacHigh = uResponseMacHigh;
  EthernetHeader->uDestMacMid = uResponseMacMid;
  EthernetHeader->uDestMacLow = uResponseMacLow;

  EthernetHeader->uSourceMacHigh = uEthernetFabricMacHigh[uId];
  EthernetHeader->uSourceMacMid = uEthernetFabricMacMid[uId];
  EthernetHeader->uSourceMacLow = uEthernetFabricMacLow[uId];

  EthernetHeader->uEthernetType = ETHERNET_TYPE_IPV4;

  IPHeader->uVersion = 0x40 | sizeof(sIPV4HeaderT) / 4;

  IPHeader->uTypeOfService = 0;
  IPHeader->uTotalLength = uResponseLength + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT);
  //IPHeader->uTotalLength = uResponseLength + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT) - 4;

  IPHeader->uFlagsFragment = 0x4000;
  IPHeader->uIdentification = uIPIdentification[uId]++;

  IPHeader->uChecksum = 0;

  IPHeader->uProtocol = IP_PROTOCOL_UDP;
  IPHeader->uTimeToLive = 0x80;

  IPHeader->uSourceIPHigh = (pIF->uIFAddrIP >> 16) & 0xFFFF;
  IPHeader->uSourceIPLow = pIF->uIFAddrIP & 0xFFFF;
  IPHeader->uDestinationIPHigh = (uResponseIPAddr >> 16) & 0xFFFF;
  IPHeader->uDestinationIPLow = uResponseIPAddr & 0xFFFF;

  uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) IPHeader);
  IPHeader->uChecksum = ~uChecksum;

  UdpHeader->uTotalLength = uResponseLength + sizeof(sUDPHeaderT);
  //UdpHeader->uTotalLength = uResponseLength + sizeof(sUDPHeaderT) - 4;
  UdpHeader->uChecksum = 0;
  UdpHeader->uSourcePort = ETHERNET_CONTROL_PORT_ADDRESS;
  UdpHeader->uDestinationPort = uResponseUDPPort;

  uChecksum = (IPHeader->uProtocol & 0x0FFu) + ((IPHeader->uTotalLength - uIPHeaderLength)
      & 0x0FFFFu);

  uChecksum += IPHeader->uSourceIPHigh + IPHeader->uSourceIPLow;
  uChecksum += IPHeader->uDestinationIPHigh + IPHeader->uDestinationIPLow;

  uChecksum = CalculateIPChecksum(uChecksum, uUdpLength / 2, (u16 *) UdpHeader);
  UdpHeader->uChecksum = ~uChecksum;

}

//=================================================================================
//  WriteRegCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the WRITE_REG command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int WriteRegCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sWriteRegReqT *Command = (sWriteRegReqT *) pCommand;
  sWriteRegRespT *Response = (sWriteRegRespT *) uResponsePacketPtr;
  u32 uReg;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sWriteRegReqT))
    return XST_FAILURE;

  uReg = ((Command->uRegDataHigh & 0xFFFF) << 16) | (Command->uRegDataLow & 0xFFFF);

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "BRD: %x ADDR: %x DATA: %x\r\n", Command->uBoardReg, Command->uRegAddress, uReg);

  // Execute the command
  if (Command->uBoardReg == 1)
    WriteBoardRegister(Command->uRegAddress, uReg);
  else
    WriteDSPRegister(Command->uRegAddress, uReg);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uBoardReg = Command->uBoardReg;
  Response->uRegAddress = Command->uRegAddress;
  Response->uRegDataHigh = Command->uRegDataHigh;
  Response->uRegDataLow = Command->uRegDataLow;

  for (uPaddingIndex = 0; uPaddingIndex < 5; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sWriteRegRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadRegCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the READ_REG command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadRegCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sReadRegReqT *Command = (sReadRegReqT *) pCommand;
  sReadRegRespT *Response = (sReadRegRespT *) uResponsePacketPtr;
  u32 uReg;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sReadRegReqT))
    return XST_FAILURE;

  // Execute the command
  if (Command->uBoardReg == 1)
    uReg = ReadBoardRegister(Command->uRegAddress);
  else
    uReg = ReadDSPRegister(Command->uRegAddress);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uBoardReg = Command->uBoardReg;
  Response->uRegAddress = Command->uRegAddress;
  Response->uRegDataHigh = (uReg >> 16) & 0xFFFF;
  Response->uRegDataLow = uReg & 0xFFFF;

  for (uPaddingIndex = 0; uPaddingIndex < 5; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sReadRegRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  WriteWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the WRITE_WISHBONE command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int WriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sWriteWishboneReqT *Command = (sWriteWishboneReqT *) pCommand;
  sWriteWishboneRespT *Response = (sWriteWishboneRespT *) uResponsePacketPtr;
  u32 uAddress;
  u32 uWriteData;
  u8 uPaddingIndex;
  u8 errno = 0;

  if (uCommandLength < sizeof(sWriteWishboneReqT))
    return XST_FAILURE;

  uAddress = (Command->uAddressHigh << 16) | (Command->uAddressLow);
  uWriteData = (Command->uWriteDataHigh << 16) | (Command->uWriteDataLow);

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_TRACE, "wb wr data 0x%08x @ 0x%08x\r\n", uWriteData, uAddress);

  // Execute the command
  /*Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress,uWriteData);*/
  WriteWishboneRegister(uAddress, uWriteData);
  /* check if an axi data bus exception was raised */
  errno = read_and_clear_error_flag();
  /* TODO: should we error out on all errno's or only on AXI_DATA_BUS errno? */
  if (errno == ERROR_AXI_DATA_BUS){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] AXI data bus error - wishbone addr outside range perhaps?\r\n", errno);
    Response->uErrorStatus = 1;
  } else {
    Response->uErrorStatus = 0;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uWriteDataHigh = Command->uWriteDataHigh;
  Response->uWriteDataLow = Command->uWriteDataLow;

  for (uPaddingIndex = 0; uPaddingIndex < 4; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sWriteWishboneRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the READ_WISHBONE command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sReadWishboneReqT *Command = (sReadWishboneReqT *) pCommand;
  sReadWishboneRespT *Response = (sReadWishboneRespT *) uResponsePacketPtr;
  u32 uAddress;
  u32 uReadData;
  u8 uPaddingIndex;
  u8 errno = 0;

  if (uCommandLength < sizeof(sReadWishboneReqT))
    return XST_FAILURE;

  uAddress = (Command->uAddressHigh << 16) | (Command->uAddressLow);

  // Execute the command
  /* uReadData = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress); */
  uReadData = ReadWishboneRegister(uAddress);
  /* check if an axi data bus exception was raised */
  errno = read_and_clear_error_flag();
  /* TODO: should we error out on all errno's or only on AXI_DATA_BUS errno? */
  if (errno == ERROR_AXI_DATA_BUS){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] AXI data bus error - wishbone addr outside range perhaps?\r\n", errno);
    Response->uErrorStatus = 1;
  } else {
    Response->uErrorStatus = 0;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uReadDataHigh = (uReadData >> 16) & 0xFFFF;
  Response->uReadDataLow = uReadData & 0xFFFF;

  for (uPaddingIndex = 0; uPaddingIndex < 4; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sReadWishboneRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  WriteI2CCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the WRITE_I2C command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int WriteI2CCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sWriteI2CReqT *Command = (sWriteI2CReqT *) pCommand;
  sWriteI2CRespT *Response = (sWriteI2CRespT *) uResponsePacketPtr;
  int iStatus;
  u8 uIndex;
  u32 uTimeout;

  /*
     if (uCommandLength < sizeof(sWriteI2CReqT))
     return XST_FAILURE;
   */

#ifdef DEBUG_PRINT
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x SLAVE ADDRESS: %x NUM BYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uNumBytes);

  //for (uIndex = 0; uIndex < Command->uNumBytes; uIndex++)
  //  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INDEX: %x DATA: %x\r\n", uIndex, Command->uWriteBytes[uIndex]);
#endif


  if ((uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT)&&((uQSFPMezzanineLocation + 1) == Command->uId))
  {
    // If we are accessing the QSFP module, set the QSFP monitoring state back to start
    //uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;
    uQSFPResetApp();

    // If doing a single write then we can expect a read will follow so disable the QSFP monitoring
    // so it doesn't interfere with the I2C address pointer
    if (Command->uNumBytes == 0x1)
      uQSFPI2CMicroblazeAccess = QSFP_I2C_MICROBLAZE_DISABLE;
    else
      uQSFPI2CMicroblazeAccess = QSFP_I2C_MICROBLAZE_ENABLE;

    // Wait until ready to access I2C
    uTimeout = 0;
    while((uQSFPUpdateStatusEnable == DO_NOT_UPDATE_QSFP_STATUS)&&(uTimeout < QSFP_I2C_ACCESS_TIMEOUT))
    {
      uTimeout++;
      Delay(1);
    }

    if (uTimeout == QSFP_I2C_ACCESS_TIMEOUT)
      iStatus = XST_FAILURE;
    else
      iStatus = WriteI2CBytes(Command->uId, Command->uSlaveAddress, Command->uWriteBytes, Command->uNumBytes);

    uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;
  }
  else
  {
    // Execute the command
    iStatus = WriteI2CBytes(Command->uId, Command->uSlaveAddress, Command->uWriteBytes, Command->uNumBytes);
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uId = Command->uId;
  Response->uSlaveAddress = Command->uSlaveAddress;
  Response->uNumBytes = Command->uNumBytes;
  for (uIndex = 0; uIndex < MAX_I2C_WRITE_BYTES; uIndex++)
    Response->uWriteBytes[uIndex] = Command->uWriteBytes[uIndex];

  if (iStatus == XST_SUCCESS)
    Response->uWriteSuccess = 1;
  else
    Response->uWriteSuccess = 0;

  //Response->uPadding[0] = 0;

  *uResponseLength = sizeof(sWriteI2CRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadI2CCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the READ_I2C command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadI2CCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sReadI2CReqT *Command = (sReadI2CReqT *) pCommand;
  sReadI2CRespT *Response = (sReadI2CRespT *) uResponsePacketPtr;
  int iStatus;
  u32 uIndex;
  u32 uTimeout;

  if (uCommandLength < sizeof(sReadI2CReqT))
    return XST_FAILURE;

#ifdef DEBUG_PRINT
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x SLAVE ADDRESS: %x NUM BYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uNumBytes);
#endif

  if ((uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT)&&((uQSFPMezzanineLocation + 1) == Command->uId))
  {
    // If we are accessing the QSFP module, set the QSFP monitoring state back to start
    //uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;
    uQSFPResetApp();

    // If doing a read then we are finished accessing
    uQSFPI2CMicroblazeAccess = QSFP_I2C_MICROBLAZE_ENABLE;

    // Wait until ready to access I2C
    uTimeout = 0;
    while((uQSFPUpdateStatusEnable == DO_NOT_UPDATE_QSFP_STATUS)&&(uTimeout < QSFP_I2C_ACCESS_TIMEOUT))
    {
      uTimeout++;
      Delay(1);
    }

    if (uTimeout == QSFP_I2C_ACCESS_TIMEOUT)
      iStatus = XST_FAILURE;
    else
      iStatus = ReadI2CBytes(Command->uId, Command->uSlaveAddress, & Response->uReadBytes[0], Command->uNumBytes);

    uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

  }
  else
  {
    // Execute the command
    iStatus = ReadI2CBytes(Command->uId, Command->uSlaveAddress, & Response->uReadBytes[0], Command->uNumBytes);
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uId = Command->uId;
  Response->uSlaveAddress = Command->uSlaveAddress;
  Response->uNumBytes = Command->uNumBytes;

#ifdef DEBUG_PRINT
  for (uIndex = 0; uIndex < Command->uNumBytes; uIndex++)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INDEX: %x READ DATA: %x\r\n", uIndex, Response->uReadBytes[uIndex]);
#endif

  if (iStatus == XST_SUCCESS)
    Response->uReadSuccess = 1;
  else
    Response->uReadSuccess = 0;

  Response->uPadding[0] = 0;

  *uResponseLength = sizeof(sReadI2CRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  SdramReconfigureCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the SDRAM_RECONFIGURE command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN    ID of the interface the command was received on
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int SdramReconfigureCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{

  sSdramReconfigureReqT *Command = (sSdramReconfigureReqT *) pCommand;
  sSdramReconfigureRespT *Response = (sSdramReconfigureRespT *) uResponsePacketPtr;
  u32 uReg;
  u8 uIndex;
  u32 uContinuityData;
  u32 uMuxSelect = 0;

  if (uCommandLength < sizeof(sSdramReconfigureReqT))
    return XST_FAILURE;

  if (Command->uDoSdramAsyncRead == 0)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "OUT: %x CLR SDR: %x FIN: %x ABT: %x RBT: %x\r\n", Command->uOutputMode, Command->uClearSdram, Command->uFinishedWritingToSdram, Command->uAboutToBootFromSdram, Command->uDoReboot);
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "RST: %x CLR: %x ENBL: %x DO RD: %x\r\n", Command->uResetSdramReadAddress, Command->uClearEthernetStatistics, Command->uEnableDebugSdramReadMode, Command->uDoSdramAsyncRead);
  }

  // Execute the command
  if ((Command->uAboutToBootFromSdram == 1)||(Command->uDoReboot == 1))
    SetOutputMode(Command->uOutputMode, 0x0); // Release flash bus when about to do a reboot
  else
    SetOutputMode(Command->uOutputMode, 0x1);

  if (Command->uOutputMode == FLASH_MODE)
  {
    // Make sure flash is in ASYNC READ MODE
    ProgramReadConfigurationRegisterCmd(0xD807);
  }

  if (Command->uClearSdram == 1){
    ClearSdram();

    /* Now, set the mux bit (bit #2) in C_WR_BRD_CTL_STAT_1_ADDR (register 0x18) to select */
    /* the appropriate interface to receive the config data on. */
    /* Since this is a later addition to this command, this seems to be the most appropriate place to do it. */
    /* i.e. after clearing the sdram and in preparation for the config data to be written. */
    /* Set bit #2 to 0 for 40gbe and 1 for 1gbe. Set bit #1 to 0 for config_data (would be set to 1 for user_data) */
    /* This is done in order for the uBlaze to detect which interface it is receiving the SDRAM configure */
    /* command over and set the board register in the fpga to the appropriate value for mux selection */
    /* therefore, set register 0x18 to 0x4 for uId 0 (1gbe i/f) and 0x0 for the rest (40gbe i/f's) */
    /* note: this register will fallback to defaults in fpga on reboot or link status change. */

    uMuxSelect = (uId == 0 ? 0x4 : 0x0);

    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + C_WR_BRD_CTL_STAT_1_ADDR, uMuxSelect);

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Received SDRAM reconfig command on %s-i/f with id %d\r\n", uId == 0 ? "1gbe" : "40gbe", uId);
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Setting board register 0x%.4x to 0x%.4x\r\n", C_WR_BRD_CTL_STAT_1_ADDR, uMuxSelect);

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "About to send IGMP leave messages.\r\n");

    /* The clear sdram command is usually called before programming. Thus, in anticipation of a new sdram image being sent */
    /* to the skarab, unsubscribe from all igmp groups. */

    // Check which Ethernet interfaces are part of IGMP groups
    // and send leave messages immediately
    for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
    {
#if 0
      if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
      {
        uIGMPState[uIndex] = IGMP_STATE_LEAVING;
        uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
        uCurrentIGMPMessage[uIndex] = 0x0;
      }
#endif
      if (XST_FAILURE == uIGMPLeaveGroup(uIndex)){
        log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] failed to leave multicast group.\r\n", uIndex);
      }
    }

    /* Disable SDRAM programming over Wishbone and clear registers */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_EN_REG_ADDRESS, 0x0);
    /* Clear the Control Register Bits */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x0);
  }

  if (Command->uFinishedWritingToSdram == 1)
    FinishedWritingToSdram();

  if (Command->uAboutToBootFromSdram == 1)
    AboutToBootFromSdram();

  // Will lose the Microblaze here if do reboot, no response packet generated
  // Could get fancy by sending response packet first and then rebooting
  // Keep it simple for now
  if (Command->uDoReboot == 1)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "About to reboot. Sending IGMP leave messages now.\r\n");

    // IcapeControllerInSystemReconfiguration();
    uDoReboot = REBOOT_REQUESTED;

    // Check which Ethernet interfaces are part of IGMP groups
    // and send leave messages immediately
    for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
    {
#if 0
      if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
      {
        uIGMPState[uIndex] = IGMP_STATE_LEAVING;
        uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
        uCurrentIGMPMessage[uIndex] = 0x0;
      }
#endif
      if (XST_FAILURE == uIGMPLeaveGroup(uIndex)){
        log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] failed to leave multicast group\r\n", uIndex);
      }
    }
  }

  if (Command->uResetSdramReadAddress == 1)
    ResetSdramReadAddress();

  if (Command->uClearEthernetStatistics == 1)
    ClearEthernetStatics();

  EnableDebugSdramReadMode(Command->uEnableDebugSdramReadMode);

  if (Command->uDoSdramAsyncRead == 1)
  {
    if (uPreviousSequenceNumber == Command->Header.uSequenceNumber)
    {
      uReg = uPreviousAsyncSdramRead;
    }
    else
    {
      uReg = ReadSdramWord();
      uPreviousAsyncSdramRead = uReg;
    }

    uPreviousSequenceNumber = Command->Header.uSequenceNumber;

#ifdef DEBUG_PRINT
    //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "READ DATA: %x\r\n", uReg);
#endif
    Response->uSdramAsyncReadDataHigh = (uReg >> 16) & 0xFFFF;
    Response->uSdramAsyncReadDataLow = uReg & 0xFFFF;
  }
  else
  {
    Response->uSdramAsyncReadDataHigh = 0x0;
    Response->uSdramAsyncReadDataLow = 0x0;
  }

  if (Command->uDoContinuityTest == 1)
  {
    uReg = (Command->uContinuityTestOutputLow) | (Command->uContinuityTestOutputHigh << 16);
    uContinuityData = ContinuityTest(uReg);

    Response->uContinuityTestReadLow = uContinuityData & 0xFFFF;
    Response->uContinuityTestReadHigh = (uContinuityData >> 16) & 0xFFFF;
  }
  else
  {
    Response->uContinuityTestReadLow = 0x0;
    Response->uContinuityTestReadHigh = 0x0;
  }

  Response->uNumEthernetFrames = GetNumEthernetFrames();
  Response->uNumEthernetBadFrames = GetNumEthernetBadFrames();
  Response->uNumEthernetOverloadFrames = GetNumEthernetOverloadFrames();

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uOutputMode = Command->uOutputMode;
  Response->uClearSdram = Command->uClearSdram;
  Response->uFinishedWritingToSdram = Command->uFinishedWritingToSdram;
  Response->uAboutToBootFromSdram = Command->uAboutToBootFromSdram;
  Response->uDoReboot = Command->uDoReboot;
  Response->uResetSdramReadAddress = Command->uResetSdramReadAddress;
  Response->uClearEthernetStatistics = Command->uClearEthernetStatistics;
  Response->uEnableDebugSdramReadMode = Command->uEnableDebugSdramReadMode;
  Response->uDoSdramAsyncRead = Command->uDoSdramAsyncRead;
  Response->uDoContinuityTest = Command->uDoContinuityTest;

  *uResponseLength = sizeof(sSdramReconfigureRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadFlashWordsCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the READ_FLASH_WORDS command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadFlashWordsCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sReadFlashWordsReqT *Command = (sReadFlashWordsReqT *) pCommand;
  sReadFlashWordsRespT *Response = (sReadFlashWordsRespT *) uResponsePacketPtr;
  u32 uIndex;
  u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sReadFlashWordsReqT))
    return XST_FAILURE;

  // Execute the command
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "NUM WRDS: %x\r\n", Command->uNumWords);
  for (uIndex = 0; uIndex < Command->uNumWords; uIndex++)
  {
    Response->uReadWords[uIndex] = ReadWord(uAddress);
    //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INDX: %x READ: %x\r\n", uIndex, Response->uReadWords[uIndex]);
    uAddress++;
  }

  for (uPaddingIndex = 0; uPaddingIndex < 2; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uNumWords = Command->uNumWords;

  *uResponseLength = sizeof(sReadFlashWordsRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ProgramFlashWordsCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the PROGRAM_FLASH_WORDS command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ProgramFlashWordsCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sProgramFlashWordsReqT *Command = (sProgramFlashWordsReqT *) pCommand;
  sProgramFlashWordsRespT *Response = (sProgramFlashWordsRespT *) uResponsePacketPtr;
  u32 uIndex;
  u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;
  int iStatus = XST_SUCCESS;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "PRGRM FLASH: %x\r\n", uAddress);

  if (uCommandLength < sizeof(sProgramFlashWordsReqT))
    return XST_FAILURE;

  if (Command->uDoBufferedProgramming == 1)
  {
    iStatus = ProgramBuffer(uAddress, Command->uWriteWords, Command->uTotalNumWords, Command->uNumWords, Command->uStartProgram, Command->uFinishProgram);
  }
  else
  {
    // A3 must be 0 when using single word programming
    if (Command->uNumWords > 8)
      return XST_FAILURE;

    for (uIndex = 0; (uIndex < Command->uNumWords)&&(iStatus == XST_SUCCESS); uIndex++)
    {
      iStatus = ProgramWord(uAddress, Command->uWriteWords[uIndex]);
      uAddress++;
    }
  }

  if (iStatus == XST_SUCCESS)
    Response->uProgramSuccess = 1;
  else
    Response->uProgramSuccess = 0;

  Response->uPadding[0] = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uNumWords = Command->uNumWords;
  Response->uDoBufferedProgramming = Command->uDoBufferedProgramming;
  Response->uStartProgram = Command->uStartProgram;
  Response->uFinishProgram = Command->uFinishProgram;

  *uResponseLength = sizeof(sProgramFlashWordsRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  EraseFlashBlockCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the ERASE_FLASH_BLOCK command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int EraseFlashBlockCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sEraseFlashBlockReqT *Command = (sEraseFlashBlockReqT *) pCommand;
  sEraseFlashBlockRespT *Response = (sEraseFlashBlockRespT *) uResponsePacketPtr;
  int iStatus;
  u8 uPaddingIndex;
  u32 uBlockAddress = (Command->uBlockAddressHigh << 16) | Command->uBlockAddressLow;

  if (uCommandLength < sizeof(sEraseFlashBlockReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "BLK ADDR: %x\r\n", uBlockAddress);

  // Execute the command
  iStatus = EraseBlock(uBlockAddress);

  for (uPaddingIndex = 0; uPaddingIndex < 6; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  if (iStatus == XST_SUCCESS)
    Response->uEraseSuccess = 1;
  else
    Response->uEraseSuccess = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uBlockAddressHigh = Command->uBlockAddressHigh;
  Response->uBlockAddressLow = Command->uBlockAddressLow;

  *uResponseLength = sizeof(sEraseFlashBlockRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadSpiPageCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the READ_SPI_PAGE command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sReadSpiPageReqT *Command = (sReadSpiPageReqT *) pCommand;
  sReadSpiPageRespT *Response = (sReadSpiPageRespT *) uResponsePacketPtr;
  int iStatus;
  u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;

  if (uCommandLength < sizeof(sReadSpiPageReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "SPI READ ADDR: %x NUM BYTES: %x\r\n", uAddress, Command->uNumBytes);

  // Execute the command
  iStatus = ISPSPIReadPage(uAddress, Response->uReadBytes, Command->uNumBytes);

  if (iStatus == XST_SUCCESS)
    Response->uReadSpiPageSuccess = 1;
  else
    Response->uReadSpiPageSuccess = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uNumBytes = Command->uNumBytes;

  Response->uPadding[0] = 0;

  *uResponseLength = sizeof(sReadSpiPageRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ProgramSpiPageCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the PROGRAM_SPI_PAGE command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ProgramSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sProgramSpiPageReqT *Command = (sProgramSpiPageReqT *) pCommand;
  sProgramSpiPageRespT *Response = (sProgramSpiPageRespT *) uResponsePacketPtr;
  int iStatus;
  u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;

  if (uCommandLength < sizeof(sProgramSpiPageReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ADDR: %x NUM BYTES: %x\r\n", uAddress, Command->uNumBytes);

  // Execute the command
  iStatus = ISPSPIProgramPage(uAddress, Command->uWriteBytes, Command->uNumBytes);

  if (iStatus == XST_SUCCESS)
  {
    Response->uProgramSpiPageSuccess = 1;

    // Now read the bytes written as verification
    iStatus = ISPSPIReadPage(uAddress, Response->uVerifyBytes, Command->uNumBytes);

    if (iStatus == XST_FAILURE)
      Response->uProgramSpiPageSuccess = 0;
  }
  else
    Response->uProgramSpiPageSuccess = 0;


  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uAddressHigh = Command->uAddressHigh;
  Response->uAddressLow = Command->uAddressLow;
  Response->uNumBytes = Command->uNumBytes;

  Response->uPadding[0] = 0;

  *uResponseLength = sizeof(sProgramSpiPageRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  EraseSpiSectorCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the ERASE_SPI_SECTOR command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int EraseSpiSectorCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sEraseSpiSectorReqT *Command = (sEraseSpiSectorReqT *) pCommand;
  sEraseSpiSectorRespT *Response = (sEraseSpiSectorRespT *) uResponsePacketPtr;
  int iStatus;
  u32 uSectorAddress = (Command->uSectorAddressHigh << 16) | Command->uSectorAddressLow;
  u8 uPaddingIndex;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "SPI SECT ERASE ADDR: %x\r\n", uSectorAddress);

  if (uCommandLength < sizeof(sEraseSpiSectorReqT))
    return XST_FAILURE;

  // Execute the command
  iStatus = ISPSPIEraseSector(uSectorAddress);

  if (iStatus == XST_SUCCESS)
    Response->uEraseSuccess = 1;
  else
    Response->uEraseSuccess = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uSectorAddressHigh = Command->uSectorAddressHigh;
  Response->uSectorAddressLow = Command->uSectorAddressLow;

  for (uPaddingIndex = 0; uPaddingIndex < 6; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sEraseSpiSectorRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  OneWireReadRomCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the ONE_WIRE_READ_ROM command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int OneWireReadRomCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sOneWireReadROMReqT *Command = (sOneWireReadROMReqT *) pCommand;
  sOneWireReadROMRespT *Response = (sOneWireReadROMRespT *) uResponsePacketPtr;
  int iStatus;
  u16 uIndex;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sOneWireReadROMReqT))
    return XST_FAILURE;

  // Execute the command
  iStatus = OneWireReadRom(Response->uRom, Command->uOneWirePort);

  for (uIndex = 0; uIndex < 8; uIndex++)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INDX: %x ROM: %x\r\n", uIndex, Response->uRom[uIndex]);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uOneWirePort = Command->uOneWirePort;

  if (iStatus == XST_SUCCESS)
    Response->uReadSuccess = 1;
  else
    Response->uReadSuccess = 0;

  for (uPaddingIndex = 0; uPaddingIndex < 3; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sOneWireReadROMRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  OneWireDS2433WriteMemCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the ONE_WIRE_DS2433_WRITE_MEM command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int OneWireDS2433WriteMemCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sOneWireDS2433WriteMemReqT *Command = (sOneWireDS2433WriteMemReqT *) pCommand;
  sOneWireDS2433WriteMemRespT *Response = (sOneWireDS2433WriteMemRespT *) uResponsePacketPtr;
  int iStatus;
  u8 uIndex;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sOneWireDS2433WriteMemReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "SKP ROM: %x, NUM BYTES: %x TA1: %x TA2: %x ID: %x", Command->uSkipRomAddress, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);

  // Execute the command
  iStatus = DS2433WriteMem(Command->uDeviceRom, Command->uSkipRomAddress, Command->uWriteBytes, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);

  if (iStatus == XST_SUCCESS)
    Response->uWriteSuccess = 1;
  else
    Response->uWriteSuccess = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  for (uIndex = 0; uIndex < 8; uIndex++)
    Response->uDeviceRom[uIndex] = Command->uDeviceRom[uIndex];

  Response->uSkipRomAddress = Command->uSkipRomAddress;

  for (uIndex = 0; uIndex < 32; uIndex++)
    Response->uWriteBytes[uIndex] = Command->uWriteBytes[uIndex];

  Response->uNumBytes = Command->uNumBytes;
  Response->uTA1 = Command->uTA1;
  Response->uTA2 = Command->uTA2;
  Response->uOneWirePort = Command->uOneWirePort;

  for (uPaddingIndex = 0; uPaddingIndex < 3; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sOneWireDS2433WriteMemRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  OneWireDS2433ReadMemCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the ONE_WIRE_DS2433_READ_MEM command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int OneWireDS2433ReadMemCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sOneWireDS2433ReadMemReqT *Command = (sOneWireDS2433ReadMemReqT *) pCommand;
  sOneWireDS2433ReadMemRespT *Response = (sOneWireDS2433ReadMemRespT *) uResponsePacketPtr;
  u8 uIndex;
  int iStatus;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sOneWireDS2433ReadMemReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "SKIP: %x NUMBYTES: %x TA1: %x TA2: %x ID: %x\r\n", Command->uSkipRomAddress, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);

  // Execute the command
  iStatus = DS2433ReadMem(Command->uDeviceRom, Command->uSkipRomAddress, Response->uReadBytes, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  for (uIndex = 0; uIndex < 8; uIndex++)
    Response->uDeviceRom[uIndex] = Command->uDeviceRom[uIndex];

  Response->uSkipRomAddress = Command->uSkipRomAddress;

  Response->uNumBytes = Command->uNumBytes;
  Response->uTA1 = Command->uTA1;
  Response->uTA2 = Command->uTA2;
  Response->uOneWirePort = Command->uOneWirePort;

  if (iStatus == XST_SUCCESS)
    Response->uReadSuccess = 1;
  else
    Response->uReadSuccess = 0;

  for (uPaddingIndex = 0; uPaddingIndex < 3; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sOneWireDS2433ReadMemRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  DebugConfigureEthernetCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the DEBUG_CONFIGURE_ETHERNET command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int DebugConfigureEthernetCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sDebugConfigureEthernetReqT *Command = (sDebugConfigureEthernetReqT *) pCommand;
  sDebugConfigureEthernetRespT *Response = (sDebugConfigureEthernetRespT *) uResponsePacketPtr;
  u32 uFabricIPAddress = (Command->uFabricIPAddressHigh << 16) | Command->uFabricIPAddressLow;
  u32 uFabricMultiCastIPAddress = (Command->uFabricMultiCastIPAddressHigh << 16) | Command->uFabricMultiCastIPAddressLow;
  u32 uFabricMultiCastIPAddressMask = (Command->uFabricMultiCastIPAddressMaskHigh << 16) | Command->uFabricMultiCastIPAddressMaskLow;

  if (uCommandLength < sizeof(sDebugConfigureEthernetReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x MAC: %x %x %x ENABLE: %x\r\n", Command->uId, Command->uFabricMacHigh, Command->uFabricMacMid, Command->uFabricMacLow, Command->uEnableFabricInterface);
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "PORT: %x GTWAY: %x %x IP: %x MCSTIP: %x MCSTIPMSK: %x\r\n", Command->uFabricPortAddress, Command->uGatewayIPAddressHigh, Command->uGatewayIPAddressLow, uFabricIPAddress, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);

  // Execute the command

  // CURRENTLY ONLY USED WHEN TESTING IN LOOPBACK AND DHCP DOESN'T COMPLETE
  // SetFabricSourceMACAddress(Command->uId, Command->uFabricMacHigh, (Command->uFabricMacMid << 16)|(Command->uFabricMacLow)); SET BASED ON SERIAL NUMBER
  // SetFabricSourcePortAddress(Command->uId, Command->uFabricPortAddress); ONLY DEFAULT VALUE USED
  //SetFabricGatewayARPCacheAddress(Command->uId, (Command->uGatewayIPAddressLow & 0xFF)); NOT NEEDED IN LOOPBACK
  SetFabricSourceIPAddress(Command->uId, uFabricIPAddress);
  //SetMultiCastIPAddress(Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask); NOT NEEDED IN LOOPBACK
  EnableFabricInterface(Command->uId, Command->uEnableFabricInterface);

  uDHCPState[Command->uId] = DHCP_STATE_COMPLETE;

  //uEthernetSubnet[Command->uId] = (uFabricIPAddress & 0xFFFFFF00);

  // Enable ARP requests now that have IP address
  // DONT DO THIS IN LOOPBACK ELSE CAUSES DETECTION OF IP ADDRESS CONFLICTS
  // (LOOPED BACK ARP REQUEST)
  //uEnableArpRequests[Command->uId] = ARP_REQUESTS_ENABLE;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uId = Command->uId;
  Response->uFabricMacHigh = Command->uFabricMacHigh;
  Response->uFabricMacMid = Command->uFabricMacMid;
  Response->uFabricMacLow = Command->uFabricMacLow;
  Response->uFabricIPAddressHigh = Command->uFabricIPAddressHigh;
  Response->uFabricIPAddressLow = Command->uFabricIPAddressLow;
  Response->uGatewayIPAddressHigh = Command->uGatewayIPAddressHigh;
  Response->uGatewayIPAddressLow = Command->uGatewayIPAddressLow;
  Response->uFabricPortAddress = Command->uFabricPortAddress;
  Response->uFabricMultiCastIPAddressHigh = Command->uFabricMultiCastIPAddressHigh;
  Response->uFabricMultiCastIPAddressLow = Command->uFabricMultiCastIPAddressLow;
  Response->uFabricMultiCastIPAddressMaskHigh = Command->uFabricMultiCastIPAddressMaskHigh;
  Response->uFabricMultiCastIPAddressMaskLow = Command->uFabricMultiCastIPAddressMaskLow;
  Response->uEnableFabricInterface = Command->uEnableFabricInterface;

  *uResponseLength = sizeof(sDebugConfigureEthernetRespT);

  //uEthernetFabricMacHigh[Command->uId] = Command->uFabricMacHigh;
  //uEthernetFabricMacMid[Command->uId] = Command->uFabricMacMid;
  //uEthernetFabricMacLow[Command->uId] = Command->uFabricMacLow;
  //uEthernetFabricIPAddress[Command->uId] = uFabricIPAddress;
  //uEthernetGatewayIPAddress[Command->uId] = (Command->uGatewayIPAddressHigh << 16) | Command->uGatewayIPAddressLow;
  //uEthernetFabricPortAddress[Command->uId] = Command->uFabricPortAddress;
  //uEthernetFabricMultiCastIPAddress[Command->uId] = uFabricMultiCastIPAddress;
  //uEthernetFabricMultiCastIPAddressMask[Command->uId] = uFabricMultiCastIPAddressMask;


  return XST_SUCCESS;
}

//=================================================================================
//  DebugAddARPCacheEntryCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the DEBUG_ADD_ARP_CACHE_ENTRY command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int DebugAddARPCacheEntryCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sDebugAddARPCacheEntryReqT *Command = (sDebugAddARPCacheEntryReqT *) pCommand;
  sDebugAddARPCacheEntryRespT *Response = (sDebugAddARPCacheEntryRespT *) uResponsePacketPtr;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sDebugAddARPCacheEntryReqT))
    return XST_FAILURE;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x INDX: %x MAC: %x %x %x\r\n", Command->uId, Command->uIPAddressLower8Bits, Command->uMacHigh, Command->uMacMid, Command->uMacLow);

  // Execute the command
  ProgramARPCacheEntry(Command->uId, Command->uIPAddressLower8Bits, Command->uMacHigh, (Command->uMacMid << 16)|(Command->uMacLow));

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uIPAddressLower8Bits = Command->uIPAddressLower8Bits;
  Response->uMacHigh = Command->uMacHigh;
  Response->uMacMid = Command->uMacMid;
  Response->uMacLow = Command->uMacLow;

  for (uPaddingIndex = 0; uPaddingIndex < 4; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sDebugAddARPCacheEntryRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  GetEmbeddedSoftwareVersionCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the GET_EMBEDDED_SOFTWARE_VERS command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int GetEmbeddedSoftwareVersionCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  u8 uPaddingIndex;

  sGetEmbeddedSoftwareVersionReqT *Command = (sGetEmbeddedSoftwareVersionReqT *) pCommand;
  sGetEmbeddedSoftwareVersionRespT *Response = (sGetEmbeddedSoftwareVersionRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetEmbeddedSoftwareVersionReqT))
    return XST_FAILURE;

  for (uPaddingIndex = 0; uPaddingIndex < 4; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uVersionMajor = EMBEDDED_SOFTWARE_VERSION_MAJOR;
  Response->uVersionMinor = EMBEDDED_SOFTWARE_VERSION_MINOR;
  Response->uVersionPatch = EMBEDDED_SOFTWARE_VERSION_PATCH;
  Response->uQSFPBootloaderVersionMajor = uQSFPBootloaderVersionMajor;
  Response->uQSFPBootloaderVersionMinor = uQSFPBootloaderVersionMinor;

  *uResponseLength = sizeof(sGetEmbeddedSoftwareVersionRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  PMBusReadI2CBytesCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the PMBUS_READ_I2C command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int PMBusReadI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sPMBusReadI2CBytesReqT *Command = (sPMBusReadI2CBytesReqT *) pCommand;
  sPMBusReadI2CBytesRespT *Response = (sPMBusReadI2CBytesRespT *) uResponsePacketPtr;
  int iStatus;

  if (uCommandLength < sizeof(sPMBusReadI2CBytesReqT))
    return XST_FAILURE;

#ifdef DEBUG_PRINT
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x SLV: %x CMD: %x NMBYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uCommandCode, Command->uNumBytes);
#endif

  // Execute the command
  iStatus = PMBusReadI2CBytes(Command->uId, Command->uSlaveAddress, Command->uCommandCode, Response->uReadBytes, Command->uNumBytes);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uId = Command->uId;
  Response->uSlaveAddress = Command->uSlaveAddress;
  Response->uCommandCode = Command->uCommandCode;
  Response->uNumBytes = Command->uNumBytes;

  if (iStatus == XST_SUCCESS)
    Response->uReadSuccess = 1;
  else
    Response->uReadSuccess = 0;

  *uResponseLength = sizeof(sPMBusReadI2CBytesRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ConfigureMulticastCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the CONFIGURE_MULTICAST command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ConfigureMulticastCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sConfigureMulticastReqT *Command = (sConfigureMulticastReqT *) pCommand;
  sConfigureMulticastRespT *Response = (sConfigureMulticastRespT *) uResponsePacketPtr;
  u32 uFabricMultiCastIPAddress;
  u32 uFabricMultiCastIPAddressMask;

  if (uCommandLength < sizeof(sConfigureMulticastReqT))
    return XST_FAILURE;

  uFabricMultiCastIPAddress = (Command->uFabricMultiCastIPAddressHigh << 16) | Command->uFabricMultiCastIPAddressLow;

  uFabricMultiCastIPAddressMask = (Command->uFabricMultiCastIPAddressMaskHigh << 16) | Command->uFabricMultiCastIPAddressMaskLow;

  log_printf(LOG_SELECT_IGMP, LOG_LEVEL_INFO, "ID: %x MC IP: %x MC MASK: %x \r\n", Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);

  // Execute the command
  SetMultiCastIPAddress(Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);

  if (XST_FAILURE == uIGMPJoinGroup(Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask)){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] failed to join multicast group with addr %08x and mask %08x \r\n", Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);
  }

#if 0
  uEthernetFabricMultiCastIPAddress[Command->uId] = uFabricMultiCastIPAddress;
  uEthernetFabricMultiCastIPAddressMask[Command->uId] = uFabricMultiCastIPAddressMask;

  // Enable sending of Multicast packets on this interface
  uIGMPState[Command->uId] = IGMP_STATE_JOINED_GROUP;
  uIGMPSendMessage[Command->uId] = IGMP_SEND_MESSAGE;
  uCurrentIGMPMessage[Command->uId] = 0x0;
#endif

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uId = Command->uId;
  Response->uFabricMultiCastIPAddressHigh = Command->uFabricMultiCastIPAddressHigh;
  Response->uFabricMultiCastIPAddressLow = Command->uFabricMultiCastIPAddressLow;
  Response->uFabricMultiCastIPAddressMaskHigh = Command->uFabricMultiCastIPAddressMaskHigh;
  Response->uFabricMultiCastIPAddressMaskLow = Command->uFabricMultiCastIPAddressMaskLow;

  *uResponseLength = sizeof(sConfigureMulticastRespT);

  return XST_SUCCESS;
}

/* TODO: verify that it's safe to remove this function */
#if 0 
//=================================================================================
//  CheckArpRequest
//--------------------------------------------------------------------------------
//  This method checks whether valid ARP request and whether ARP request is for us.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId         IN  ID of network interface that received ARP request
//  uFabricIPAddress  IN  Our IP address
//  uPktLen       IN  Length of packet
//  pArpPacket      IN  Pointer to packet
//
//  Return
//  ------
//  XST_SUCCESS if successful match
//=================================================================================
int CheckArpRequest(u8 uId, u32 uFabricIPAddress, u32 uPktLen, u8 *pArpPacket)
{
  u32 uSenderIPAddress;

  if (uPktLen < sizeof(sArpPacketT))
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ARP Length problem\r\n");
    return XST_FAILURE;
  }

  struct sArpPacket *Arp = (struct sArpPacket *) pArpPacket;

  if (Arp->uHardwareType != 1 || Arp->uProtocolType != ETHERNET_TYPE_IPV4)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ARP Type problem\r\n");
    return XST_FAILURE;
  }

  if (Arp->uHardwareLength != 6 || Arp->uProtocolLength != 4)
  {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ARP Hardware length problem\r\n");
    return XST_FAILURE;
  }

  if (Arp->uTargetIPHigh != ((uFabricIPAddress >> 16) & 0xFFFFu) ||
      Arp->uTargetIPLow != (uFabricIPAddress & 0xFFFFu))
    return XST_FAILURE;

  if (Arp->uOperation != 1)
  {
    // Check that IP address of sender does not match our IP address (else we have an IP
    // address conflict so we should start DHCP all over again
    uSenderIPAddress = (Arp->uSenderIpHigh << 16) | Arp->uSenderIPLow;

    if (uSenderIPAddress == uEthernetFabricIPAddress[uId])
    {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IP ADDRESS CONFLICT DETECTED! RESTARTING DHCP ON ID: %x\r\n", uId);
      uDHCPState[uId] = DHCP_STATE_DISCOVER;
      uDHCPRetryTimer[uId] = DHCP_RETRY_ENABLED;
    }
    else
    {
      // It was a response so add it to the ARP cache
#ifdef DEBUG_PRINT
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ARP ENTRY ID: %x IP: %x %x MAC: %x %x %x\r\n", uId, Arp->uSenderIpHigh, Arp->uSenderIPLow, Arp->uSenderMacHigh, Arp->uSenderMacMid, Arp->uSenderMacLow);
#endif

      ProgramARPCacheEntry(uId, (Arp->uSenderIPLow & 0xFF), Arp->uSenderMacHigh, ((Arp->uSenderMacMid << 16) | Arp->uSenderMacLow));
    }

    return XST_FAILURE;
  }

  return XST_SUCCESS;
}
#endif

#if 0
//=================================================================================
//  ArpHandler
//--------------------------------------------------------------------------------
//  This method creates the ARP response packet.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of selected Ethernet interface
//  uType     IN  1 for request, 2 for response
//  pReceivedArp  IN  Pointer to received ARP packet
//  pTransmitBuffer OUT Pointer to transmit buffer where ARP response is created
//  uResponseLength OUT Size of ARP packet created
//  uRequestedIPAddress IN  For a request, the IP address that trying to get MAC address for
//
//  Return
//  ------
//  None
//=================================================================================
void ArpHandler(u8 uId, u8 uType, u8 *pReceivedArp, u8 *pTransmitBuffer, u32 * uResponseLength, u32 uRequestedIPAddress)
{
  struct sArpPacket *ReceivedArp = (struct sArpPacket *) pReceivedArp;

  struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) pTransmitBuffer;

  u32 uIndex;

  if (uType == ARP_RESPONSE)
  {
    EthernetHeader->uDestMacHigh = uResponseMacHigh;
    EthernetHeader->uDestMacMid = uResponseMacMid;
    EthernetHeader->uDestMacLow = uResponseMacLow;
  }
  else // ARP_REQUEST
  {
    EthernetHeader->uDestMacHigh = 0xFFFF;
    EthernetHeader->uDestMacMid = 0xFFFF;
    EthernetHeader->uDestMacLow = 0xFFFF;
  }

  EthernetHeader->uSourceMacHigh = uEthernetFabricMacHigh[uId];
  EthernetHeader->uSourceMacMid = uEthernetFabricMacMid[uId];
  EthernetHeader->uSourceMacLow = uEthernetFabricMacLow[uId];

  EthernetHeader->uEthernetType = ETHERNET_TYPE_ARP;

  struct sArpPacket *TransmitArp = (struct sArpPacket *) ((u8*) pTransmitBuffer + sizeof(sEthernetHeaderT));

  // populate
  TransmitArp->uProtocolType = ETHERNET_TYPE_IPV4;
  TransmitArp->uHardwareType = 1;

  TransmitArp->uOperation = uType;
  TransmitArp->uProtocolLength = 4;
  TransmitArp->uHardwareLength = 6;

  TransmitArp->uSenderMacHigh = uEthernetFabricMacHigh[uId];
  TransmitArp->uSenderMacMid = uEthernetFabricMacMid[uId];
  TransmitArp->uSenderMacLow = uEthernetFabricMacLow[uId];

  TransmitArp->uSenderIpHigh = (uEthernetFabricIPAddress[uId] >> 16) & 0x0FFFFu;
  TransmitArp->uSenderIPLow = (uEthernetFabricIPAddress[uId] & 0x0FFFFu);

  if (uType == ARP_RESPONSE)
  {
    TransmitArp->uTargetMacHigh = uResponseMacHigh;
    TransmitArp->uTargetMacMid = uResponseMacMid;
    TransmitArp->uTargetMacLow = uResponseMacLow;

    TransmitArp->uTargetIPHigh = ReceivedArp->uSenderIpHigh;
    TransmitArp->uTargetIPLow = ReceivedArp->uSenderIPLow;
  }
  else // ARP_REQUEST
  {
    TransmitArp->uTargetMacHigh = 0x0000;
    TransmitArp->uTargetMacMid = 0x0000;
    TransmitArp->uTargetMacLow = 0x0000;

    TransmitArp->uTargetIPHigh = (uRequestedIPAddress >> 16) & 0xFFFF;
    TransmitArp->uTargetIPLow = uRequestedIPAddress & 0xFFFF;
  }

  for (uIndex = 0; uIndex < 11; uIndex++)
    TransmitArp->uPadding[uIndex] = 0;

  * uResponseLength = sizeof(sEthernetHeaderT) + sizeof(sArpPacketT);

}
#endif

//=================================================================================
//  CreateIGMPPacket
//--------------------------------------------------------------------------------
//  This method is used to create the two types of IGMP messages to the router/switch.
//  Instead of handling query messages, membership reports packets are generated
//  every 60 seconds.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId         IN  Selected Ethernet interface
//  pTransmitBuffer   OUT Location of transmit buffer (to create IGMP packet)
//  uResponseLength   OUT Length of IGMP packet
//  uMessageType    IN  Whether it is a membership report or leave message
//  uGroupAddress   IN  Address for IGMP message
//
//  Return
//  ------
//  None
//=================================================================================
void CreateIGMPPacket(u8 uId, u8 *pTransmitBuffer, u32 * uResponseLength, u8 uMessageType, u32 uGroupAddress)
{
  struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) pTransmitBuffer;
  u8 * uIPV4Header = pTransmitBuffer + sizeof(sEthernetHeaderT);
  struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPV4Header;
  u8 * uIPV4HeaderOptions = uIPV4Header + sizeof(sIPV4HeaderT); 
  struct sIPV4HeaderOptions *IPHeaderOptions = (struct sIPV4HeaderOptions *) uIPV4HeaderOptions;
  u8 * uIGMPHeader = uIPV4HeaderOptions + sizeof(sIPV4HeaderOptionsT);
  struct sIGMPHeader * IGMPHeader = (struct sIGMPHeader *) uIGMPHeader;
  struct sIFObject *pIF;

  u8 uIndex;
  u32 uChecksum;
  u32 uIPHeaderLength = sizeof(sIPV4HeaderT) + sizeof(sIPV4HeaderOptionsT);
  u32 uIGMPHeaderLength = sizeof(sIGMPHeaderT);

  u32 uTempGroupAddress;

  pIF = lookup_if_handle_by_id(uId);

  if (uMessageType == IGMP_MEMBERSHIP_REPORT)
    uTempGroupAddress = uGroupAddress;
  else
    uTempGroupAddress = IGMP_ALL_ROUTERS_IP_ADDRESS;

  uTempGroupAddress = uTempGroupAddress & IGMP_IP_MASK;

  EthernetHeader->uDestMacHigh = IGMP_MAC_ADDRESS_HIGH;
  EthernetHeader->uDestMacMid = ((uTempGroupAddress >> 16) & 0xFFFF) | IGMP_MAC_ADDRESS_MID;
  EthernetHeader->uDestMacLow = (uTempGroupAddress & 0xFFFF);

  EthernetHeader->uSourceMacHigh = uEthernetFabricMacHigh[uId];
  EthernetHeader->uSourceMacMid = uEthernetFabricMacMid[uId];
  EthernetHeader->uSourceMacLow = uEthernetFabricMacLow[uId];

  EthernetHeader->uEthernetType = ETHERNET_TYPE_IPV4;

  IPHeader->uVersion = 0x40 | (sizeof(sIPV4HeaderT) + sizeof(sIPV4HeaderOptionsT)) / 4;

  IPHeader->uTypeOfService = 0;
  IPHeader->uTotalLength = sizeof(sIPV4HeaderT) + sizeof(sIPV4HeaderOptionsT) + sizeof(sIGMPHeaderT) - 18;  /* minus the padding bytes = 9 x 2 */
  /* RFC 894 - Frame Format - */
  /* par.2 padding bytes not part of IP packet ie IP Total Length */

  IPHeader->uFlagsFragment = 0x4000;
  IPHeader->uIdentification = uIPIdentification[uId]++;

  IPHeader->uChecksum = 0;

  IPHeader->uProtocol = IP_PROTOCOL_IGMP;
  IPHeader->uTimeToLive = 0x01; // IGMP TTL = 1

  IPHeader->uSourceIPHigh = ((pIF->uIFAddrIP >> 16) & 0xFFFF);
  IPHeader->uSourceIPLow = (pIF->uIFAddrIP & 0xFFFF);

  if (uMessageType == IGMP_MEMBERSHIP_REPORT)
    uTempGroupAddress = uGroupAddress;
  else
    uTempGroupAddress = IGMP_ALL_ROUTERS_IP_ADDRESS;

  IPHeader->uDestinationIPHigh = ((uTempGroupAddress >> 16) & 0xFFFF);
  IPHeader->uDestinationIPLow = (uTempGroupAddress & 0xFFFF);

  /* RFC 2236 - last par on pg. 23: "The IGMPv2 spec requires the presence of the IP Router Alert option
     [RFC 2113] in all packets described in this memo." */

  /* Router Alert Option - RFC 2113 */
  IPHeaderOptions->uOptionsHigh = 0x9404;  /* Copied flag = 1; Option class = 0; Option Number = 20; Length = 4; */
  IPHeaderOptions->uOptionsLow = 0;  /* Router shall examine packet */

  uChecksum = CalculateIPChecksum(0, (uIPHeaderLength + 1) / 2, (u16 *) IPHeader);
  IPHeader->uChecksum = ~uChecksum;

  IGMPHeader->uType = uMessageType;
  IGMPHeader->uMaximumResponseTime = 0x0;
  IGMPHeader->uChecksum = 0x0;

  IGMPHeader->uGroupAddressHigh = ((uGroupAddress >> 16) & 0xFFFF);
  IGMPHeader->uGroupAddressLow = (uGroupAddress & 0xFFFF);

  for (uIndex = 0; uIndex < 9; uIndex++)
    IGMPHeader->uPadding[uIndex] = 0x0;

  uChecksum = CalculateIPChecksum(0, (uIGMPHeaderLength + 1) / 2, (u16 *) IGMPHeader);
  IGMPHeader->uChecksum = ~uChecksum;

  * uResponseLength = sizeof(sIGMPHeaderT) + sizeof(sIPV4HeaderOptionsT) + sizeof(sIPV4HeaderT) + sizeof(sEthernetHeaderT);
}

//=================================================================================
//  DebugLoopbackTestCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the DEBUG_LOOPBACK_TEST command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int DebugLoopbackTestCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sDebugLoopbackTestReqT *Command = (sDebugLoopbackTestReqT *) pCommand;
  sDebugLoopbackTestRespT *Response = (sDebugLoopbackTestRespT *) uResponsePacketPtr;
  int iStatus;
  unsigned uTimeout = 0x0;

  struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) uLoopbackTransmitBuffer;
  u8 * uIPResponseHeader = (u8 *) uLoopbackTransmitBuffer + sizeof(sEthernetHeaderT);
  struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPResponseHeader;
  u8 * uUdpResponseHeader = uIPResponseHeader + sizeof(sIPV4HeaderT);
  struct sUDPHeader * UdpHeader = (struct sUDPHeader *) uUdpResponseHeader;
  u8 * uDebugLoopbackTestReq = uUdpResponseHeader + sizeof(sUDPHeaderT);
  struct sDebugLoopbackTestReq * DebugLoopbackTestReq =  (struct sDebugLoopbackTestReq *) uDebugLoopbackTestReq;

  u8 * uDebugLoopbackTestReqResult =  (u8 *) uLoopbackReceiveBuffer + sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT);
  struct sDebugLoopbackTestReq * DebugLoopbackTestReqResult =  (struct sDebugLoopbackTestReq *) uDebugLoopbackTestReqResult;

  u32 uChecksum;
  u32 uIPHeaderLength = sizeof(sIPV4HeaderT);
  u32 uUdpLength = sizeof(sDebugLoopbackTestReqT) + sizeof(sUDPHeaderT);
  u8 uIndex;
  u32 uTransmitNumWords = ((sizeof(sDebugLoopbackTestReqT) + sizeof(sUDPHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sEthernetHeaderT)) >> 2);
  u32 uReceivedNumWords;
  struct sIFObject *pIF;

  if (uCommandLength < sizeof(sDebugLoopbackTestReqT))
    return XST_FAILURE;

  pIF = lookup_if_handle_by_id(Command->uId);

  // Create a packet to send to Ethernet I/F being tested

  EthernetHeader->uDestMacHigh = 0xFFFF;
  EthernetHeader->uDestMacMid = 0xFFFF;
  EthernetHeader->uDestMacLow = 0xFFFF;

  EthernetHeader->uSourceMacHigh = 0x0000;
  EthernetHeader->uSourceMacMid = 0x0000;
  EthernetHeader->uSourceMacLow = 0x0000;

  EthernetHeader->uEthernetType = ETHERNET_TYPE_IPV4;

  IPHeader->uVersion = 0x40 | sizeof(sIPV4HeaderT) / 4;

  IPHeader->uTypeOfService = 0;
  IPHeader->uTotalLength = sizeof(sDebugLoopbackTestReqT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT);

  IPHeader->uFlagsFragment = 0x4000;
  IPHeader->uIdentification = uIPIdentification[Command->uId]++;

  IPHeader->uChecksum = 0;

  IPHeader->uProtocol = IP_PROTOCOL_UDP;
  IPHeader->uTimeToLive = 0x80;

  // Loopback so IP address matches source address
  IPHeader->uSourceIPHigh = (pIF->uIFAddrIP >> 16) & 0xFFFF;
  IPHeader->uSourceIPLow = pIF->uIFAddrIP & 0xFFFF;
  IPHeader->uDestinationIPHigh = (pIF->uIFAddrIP >> 16) & 0xFFFF;
  IPHeader->uDestinationIPLow = pIF->uIFAddrIP & 0xFFFF;

  uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) IPHeader);
  IPHeader->uChecksum = ~uChecksum;

  UdpHeader->uTotalLength = sizeof(sDebugLoopbackTestReqT) + sizeof(sUDPHeaderT);
  UdpHeader->uChecksum = 0;
  UdpHeader->uSourcePort = ETHERNET_CONTROL_PORT_ADDRESS;
  UdpHeader->uDestinationPort = ETHERNET_CONTROL_PORT_ADDRESS;

  DebugLoopbackTestReq->Header.uCommandType = Command->Header.uCommandType;
  DebugLoopbackTestReq->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  DebugLoopbackTestReq->uId = Command->uId;

  for (uIndex = 0; uIndex < 256; uIndex++)
    DebugLoopbackTestReq->uTestData[uIndex] = Command->uTestData[uIndex];

  uChecksum = (IPHeader->uProtocol & 0x0FFu) + ((IPHeader->uTotalLength - uIPHeaderLength)
      & 0x0FFFFu);

  uChecksum += IPHeader->uSourceIPHigh + IPHeader->uSourceIPLow;
  uChecksum += IPHeader->uDestinationIPHigh + IPHeader->uDestinationIPLow;

  uChecksum = CalculateIPChecksum(uChecksum, uUdpLength / 2, (u16 *) UdpHeader);
  UdpHeader->uChecksum = ~uChecksum;

  // Now transmit the packet on the selected Ethernet interface
  iStatus = TransmitHostPacket(Command->uId, & uLoopbackTransmitBuffer[0], uTransmitNumWords);

  if (iStatus == XST_FAILURE)
  {
    Response->uValid = 0x0;
  }
  else
  {
    // Now wait for loopback packet with a timeout so we don't wait forever
    do
    {
      uReceivedNumWords = GetHostReceiveBufferLevel(Command->uId);
      uTimeout++;
    } while ((uReceivedNumWords == 0x0)&&(uTimeout < 1000));

    if ((uTimeout == 1000)||(uReceivedNumWords != uTransmitNumWords))
    {
      Response->uValid = 0x0;
    }
    else
    {
      iStatus  = ReadHostPacket(Command->uId, & uLoopbackReceiveBuffer[0], uReceivedNumWords);

      if (iStatus == XST_FAILURE)
      {
        Response->uValid = 0x0;
      }
      else
      {
        Response->uValid = 0x1;

        for (uIndex = 0; uIndex < 256; uIndex++)
          Response->uTestData[uIndex] = DebugLoopbackTestReqResult->uTestData[uIndex];

      }
    }

  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uId = Command->uId;

  *uResponseLength = sizeof(sDebugLoopbackTestRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  QSFPResetAndProgramCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the QSFP_RESET_AND_PROG command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int QSFPResetAndProgramCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  /*
   * FIXME: currently this cmd only makes provision to reset one mezzanine card.
   * A change to this cmd will have to be made if multiple qsfp+ modules are to
   * be supported.
   */
  static u8 btldr_programming = 0;

  sQSFPResetAndProgramReqT *Command = (sQSFPResetAndProgramReqT *) pCommand;
  sQSFPResetAndProgramRespT *Response = (sQSFPResetAndProgramRespT *) uResponsePacketPtr;
  u8 uPaddingIndex;
  u32 uReg = uWriteBoardShadowRegs[C_WR_MEZZANINE_CTL_ADDR >> 2];

  if (uCommandLength < sizeof(sQSFPResetAndProgramReqT)){
    return XST_FAILURE;
  }

  if (Command->uReset == 0x1){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Resetting Mezzanine.\r\n", uQSFPMezzanineLocation);

    uReg = uReg | (QSFP_MEZZANINE_RESET << uQSFPMezzanineLocation);
    WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, uReg);
    Delay(100);
    uReg = uReg & (~ (QSFP_MEZZANINE_RESET << uQSFPMezzanineLocation));
    WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, uReg);

    uQSFPResetInit();
    /* global variable "task" flag in main - will be set again on next timer
     * interrupt */
    uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Resetting links.\r\n", uQSFPMezzanineLocation);

    uQSFPCtrlReg = QSFP0_RESET | QSFP1_RESET | QSFP2_RESET | QSFP3_RESET;
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);

    if (Command->uProgram == 0x1){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Mezzanine entering bootloader programming mode.\r\n", uQSFPMezzanineLocation);
      btldr_programming = 1;
      uQSFPStateMachinePause();
    }
  }

  if ((Command->uProgram == 0x0)&&(btldr_programming == 1)){
    /* If we were in programming mode and notified via cmd that programming is
     now finished */
    uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Resetting links.\r\n", uQSFPMezzanineLocation);

    uQSFPCtrlReg = QSFP0_RESET | QSFP1_RESET | QSFP2_RESET | QSFP3_RESET;
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);

    uQSFPStateMachineResume();

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Mezzanine leaving bootloader programming mode.\r\n", uQSFPMezzanineLocation);
    btldr_programming = 0;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uReset = Command->uReset;
  Response->uProgram = Command->uProgram;

  for (uPaddingIndex = 0; uPaddingIndex < 7; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sQSFPResetAndProgramRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  HMCReadI2CBytesCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the HMC_READ_I2C command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int HMCReadI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  sHMCReadI2CBytesReqT *Command = (sHMCReadI2CBytesReqT *) pCommand;
  sHMCReadI2CBytesRespT *Response = (sHMCReadI2CBytesRespT *) uResponsePacketPtr;
  int iStatus;
  u8 uPaddingIndex;
  u8 uByteIndex;

  if (uCommandLength < sizeof(sHMCReadI2CBytesReqT))
    return XST_FAILURE;

#ifdef DEBUG_PRINT
  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ID: %x SLV: %x ADDRS: %x %x %x %x\r\n", Command->uId, Command->uSlaveAddress, Command->uReadAddress[0], Command->uReadAddress[1], Command->uReadAddress[2], Command->uReadAddress[3]);
#endif

  // Execute the command
  iStatus = HMCReadI2CBytes(Command->uId, Command->uSlaveAddress, Command->uReadAddress, & Response->uReadBytes[0]);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uId = Command->uId;
  Response->uSlaveAddress = Command->uSlaveAddress;

  if (iStatus == XST_SUCCESS)
    Response->uReadSuccess = 1;
  else
    Response->uReadSuccess = 0;

  for (uByteIndex = 0; uByteIndex < 4; uByteIndex++)
    Response->uReadAddress[uByteIndex] = Command->uReadAddress[uByteIndex];

  for (uPaddingIndex = 0; uPaddingIndex < 2; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sHMCReadI2CBytesRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  HMCWriteI2CBytesCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the HMC_WRITE_I2C command.
//
//  Parameter           Dir   Description
//  ---------           ---   -----------
//  pCommand            IN    Pointer to command header
//  uCommandLength      IN    Length of command
//  uResponsePacketPtr  IN    Pointer to where response packet must be constructed
//  uResponseLength     OUT   Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int HMCWriteI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  int iStatus;
  int uIndex;
  /* u32 uData, uAddr; */
  u16 uWriteBytes[8];
  u8 uPaddingIndex;
  u8 uByteIndex;

  sHMCWriteI2CBytesReqT *Command = (sHMCWriteI2CBytesReqT *) pCommand;
  sHMCWriteI2CBytesRespT *Response = (sHMCWriteI2CBytesRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sHMCWriteI2CBytesReqT)){
    return XST_FAILURE;
  }

#if 0
  /* pack the addr and data into 32-bit words */
  uAddr =         ((Command->uWriteAddress[0] & 0xff) << 24);
  uAddr = uAddr | ((Command->uWriteAddress[1] & 0xff) << 16);
  uAddr = uAddr | ((Command->uWriteAddress[2] & 0xff) <<  8);
  uAddr = uAddr | ((Command->uWriteAddress[3] & 0xff));
  uData =         ((Command->uWriteData[0] & 0xff) << 24);
  uData = uData | ((Command->uWriteData[1] & 0xff) << 16);
  uData = uData | ((Command->uWriteData[2] & 0xff) <<  8);
  uData = uData | ((Command->uWriteData[3] & 0xff));

  //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC WR[%x]: addr = %08x and data = %08x\n", Command->uId, uAddr, uData);
#endif

  //iStatus = HMCWriteI2CBytes(Command->uId, Command->uSlaveAddress, uAddr, uData);

  /* pack the addr_bytes[4] and data_bytes[4] into a single 8-byte array */
  for (uIndex = 0; uIndex < 4; uIndex++){
    uWriteBytes[uIndex] = Command->uWriteAddress[uIndex];
    uWriteBytes[uIndex + 4] = Command->uWriteData[uIndex];
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC WR[%x]:", Command->uId);
  for (uIndex = 0; uIndex < 8; uIndex++){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, " %x", uWriteBytes[uIndex]);
  }
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "\r\n");

  /* write the array of 8 bytes out directly to the i2c bus. */
  iStatus = WriteI2CBytes(Command->uId, Command->uSlaveAddress, uWriteBytes, 8);

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
  Response->uId = Command->uId;
  Response->uSlaveAddress = Command->uSlaveAddress;

  for (uByteIndex = 0; uByteIndex < 4; uByteIndex++){
    Response->uWriteAddress[uByteIndex] = Command->uWriteAddress[uByteIndex];
    Response->uWriteData[uByteIndex] = Command->uWriteData[uByteIndex];
  }

  if (iStatus == XST_SUCCESS){
    Response->uWriteSuccess = 1;
  } else {
    Response->uWriteSuccess = 0;
  }

  for (uPaddingIndex = 0; uPaddingIndex < 2; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  *uResponseLength = sizeof(sHMCWriteI2CBytesRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  SDRAMProgramOverWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the SDRAM_PROGRAM_OVER_WISHBONE command.
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int SDRAMProgramOverWishboneCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  u8 uPaddingIndex;
  u16 uChunkByteIndex;
  u32 uTemp = 0;
  u8 uRetVal;
  u8 uIndex;

  /* State variables */
  /* static u8 uCurrentProgrammingId; */      /* the interface Id we are currently receiving sdram data on */
  static u32 uChunkIdCached = 0;        /* the last chunk that has been succefully programmed */
  static u32 uChunkSizeBytesCached = 0;      /* cache the chunk length which is obtained from chunk 0 */

  sSDRAMProgramOverWishboneReqT *Command = (sSDRAMProgramOverWishboneReqT *) pCommand;
  sSDRAMProgramOverWishboneRespT *Response = (sSDRAMProgramOverWishboneRespT *) uResponsePacketPtr;

#if 0
  if (uCommandLength < sizeof(sSDRAMProgramOverWishboneReqT)){
    return XST_FAILURE;
  }
#endif

  /* only support (+-) 2k, 4k and 8k programming modes */
  /* JUST A NOTE: chunk length must be even for +2 step increment of upcoming for loop */
  if (((uCommandLength - 8) != 1988) && ((uCommandLength - 8) != 3976) && ((uCommandLength - 8) != 7952)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "SDRAM PROGRAM[%02x] Unsupported chunk size of %d bytes.\r\n" , uId, uCommandLength - 8);
    return XST_FAILURE;
  }

  /* check that the chunk size matches the previously cached size set by chunk 0 */
  if ((Command->uChunkNum != 0) && ((uCommandLength - 8) != uChunkSizeBytesCached)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "SDRAM PROGRAM[%02x] Chunk size mismatch - %d expected but %d received.\r\n" , uId, uChunkSizeBytesCached, uCommandLength - 8);
    return XST_FAILURE;
  }

  for (uPaddingIndex = 0; uPaddingIndex < 7; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uChunkNum = Command->uChunkNum; 
  Response->uStatus = 0x0;  /* ACK */

  *uResponseLength = sizeof(sSDRAMProgramOverWishboneRespT);

  /* Chunk number 0 is special case, resets everything */
  if(Command->uChunkNum == 0x0){
    uChunkSizeBytesCached = uCommandLength - 8;

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "IGMP [%02d] About to send IGMP leave messages.\r\n", uId);

#if 0
    // Check which Ethernet interfaces are part of IGMP groups
    // and send leave messages immediately
    for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
    {
#if 0
      if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
      {
        uIGMPState[uIndex] = IGMP_STATE_LEAVING;
        uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
        uCurrentIGMPMessage[uIndex] = 0x0;
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IGMP[%02x]: About to send IGMP leave message.\r\n", uIndex);
      }
#endif
      if (XST_FAILURE == uIGMPLeaveGroup(uIndex)){
        log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] failed to leave multicast group\r\n", uIndex);
      }
    }
#endif

    /* TODO - should we unsubscribe on all interfaces?? See code commented out above */
    if (XST_FAILURE == uIGMPLeaveGroup(uId)){
      log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02d] failed to leave multicast group\r\n", uId);
    }

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "SDRAM PROGRAM[%02x] Chunk 0: about to clear sdram. Detected chunk size of %d bytes.\r\n" , uId, uChunkSizeBytesCached);

    /* during programming, we need the serial console to be relatively quiet
     * since this is a data-intensive task, cache the log-level...restore later */
    cache_log_level();
    /* only print warnings during programming */
    set_log_level(LOG_LEVEL_WARN);

    uChunkIdCached = 0;
    ClearSdram();
    SetOutputMode(0x1, 0x1);
    /* Enable SDRAM Programming via Wishbone Bus */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_EN_REG_ADDRESS, 0x1);
    /* Set the Start SDRAM Programming control bit */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x2);

    uRetVal = XST_SUCCESS;

  } else if (Command->uChunkNum == (uChunkIdCached + 1)){
    /* log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "chunk %d: about to write to sdram\r\n", Command->uChunkNum); */  /* this adds lots of overhead */
    for (uChunkByteIndex = 0; uChunkByteIndex < (uChunkSizeBytesCached / 2) /*16bit words*/; uChunkByteIndex = uChunkByteIndex + 2){
      uTemp = (Command->uBitstreamChunk[uChunkByteIndex] << 16 & 0xFFFF0000) | (Command->uBitstreamChunk[uChunkByteIndex + 1] & 0x0000FFFF);

      /* Write the 32-bit data word via the Wishbone Bus */
      Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_DATA_WR_REG_ADDRESS, uTemp);

      /* TODO: Here we wait for the ACK bit to be set by the firmware but this may not to be necessary - remove to reduce overhead???*/
      /* 0x3 => bit 0 = 1 (ack) and bit 1 = 1 (start program control bit previously set) */
      while (Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR  + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS) != 0x3)

        /* Clear the ACK bit */
        Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x2);
    }

    if (Command->uChunkNum == Command->uChunkTotal){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "SDRAM PROGRAM[%02x] Chunk %d: about to end sdram write.\r\n", uId, Command->uChunkNum);

      /* restore log-level cached at start of programming */
      restore_log_level();

      SetOutputMode(0x2, 0x1);
      FinishedWritingToSdram();

      /* Set the Stop SDRAM Programming Control Bit */
      Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x4);
      /* Disable SDRAM Programming via Wishbone Bus */
      Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_EN_REG_ADDRESS, 0x0);
      /* Clear the Control Register Bits */
      Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x0);
    }
    uChunkIdCached = Command->uChunkNum;

    uRetVal = XST_SUCCESS;

  } else if (Command->uChunkNum == uChunkIdCached){
    uRetVal = XST_SUCCESS;
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "SDRAM PROGRAM[%02x] Chunk %d: already received\r\n", uId, Command->uChunkNum);
  } else {
    uRetVal = XST_FAILURE;
  }

  return uRetVal;
}

int SetDHCPTuningDebugCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  u16 data[4] = {0};
  u16 rom[8];
  u8 uPaddingIndex;
  struct sIFObject *pIFObj;

  sSetDHCPTuningDebugReqT *Command = (sSetDHCPTuningDebugReqT *) pCommand;
  sSetDHCPTuningDebugRespT *Response = (sSetDHCPTuningDebugRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sSetDHCPTuningDebugReqT)){
    return XST_FAILURE;
  }

  /* status: send 0 for failure, 1 for success */
  Response->uStatus = 1;

  pIFObj = lookup_if_handle_by_id(uId);

  if (uDHCPSetRetryInterval(pIFObj, Command->uRetryTime) == DHCP_RETURN_FAIL){
    Response->uStatus = 0;
  }

  if (uDHCPSetInitWait(pIFObj, Command->uInitTime) == DHCP_RETURN_FAIL){
    Response->uStatus = 0;
  }

  /* store the values in page15 of DS2433 one-wire EEPROM on Motherboard
     (need two bytes each)
     -> Init wait time at addr 0x1E0(LSB) and 0x1E1(MSB)
     -> Retry time at addr 0x1E2(LSB) and 0x1E3(MSB)
   */

  /* pack the data */
  data[0] = Command->uInitTime & 0xFF;
  data[1] = (Command->uInitTime >> 8) & 0xFF;
  data[2] = Command->uRetryTime & 0xFF;
  data[3] = (Command->uRetryTime >> 8) & 0xFF;

  if (Response->uStatus != 0){
    if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) != XST_SUCCESS){
      Response->uStatus = 0;
    } else {
      if (DS2433WriteMem(rom, 0, data, 4, 0xE0, 0x1, MB_ONE_WIRE_PORT) != XST_SUCCESS){
        Response->uStatus = 0;
      }
    }
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  Response->uInitTime = Command->uInitTime; 
  Response->uRetryTime = Command->uRetryTime; 

  for (uPaddingIndex = 0; uPaddingIndex < 6; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  *uResponseLength = sizeof(sSetDHCPTuningDebugRespT);

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "DHCP TUNING[%02x] %s - Retry: %d, Init: %d\r\n", pIFObj->uIFEthernetId, Response->uStatus == 0 ? "FAIL" : "OK",
      Command->uRetryTime, Command->uInitTime);

  return XST_SUCCESS;
}


int GetDHCPTuningDebugCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  u16 data[4] = {0};
  u16 rom[8];
  u8 uPaddingIndex;

  sGetDHCPTuningDebugReqT *Command = (sGetDHCPTuningDebugReqT *) pCommand;
  sGetDHCPTuningDebugRespT *Response = (sGetDHCPTuningDebugRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetDHCPTuningDebugReqT)){
    return XST_FAILURE;
  }

  /* status: send 0 for failure, 1 for success */
  Response->uStatus = 1;

  /* read the values in page15 of DS2433 one-wire EEPROM on Motherboard
     (two bytes each)
     -> Init wait time at addr 0x1E0(LSB) and 0x1E1(MSB)
     -> Retry time at addr 0x1E2(LSB) and 0x1E3(MSB)
   */

  if (Response->uStatus != 0){
    if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) != XST_SUCCESS){
      Response->uStatus = 0;
    } else {
      if (DS2433ReadMem(rom, 0, data, 4, 0xE0, 0x1, MB_ONE_WIRE_PORT) != XST_SUCCESS){
        Response->uStatus = 0;
      }
    }
  }

  if (Response->uStatus != 0){
    /* unpack the data */
    Response->uInitTime = data[1];
    Response->uInitTime = Response->uInitTime << 8;
    Response->uInitTime = Response->uInitTime | (data[0] & 0xff);
    Response->uRetryTime = data[3];
    Response->uRetryTime = Response->uRetryTime << 8;
    Response->uRetryTime = Response->uRetryTime | (data[2] & 0xff);
  } else {
    Response->uInitTime = 0;
    Response->uRetryTime = 0;
  } 

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  for (uPaddingIndex = 0; uPaddingIndex < 6; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  *uResponseLength = sizeof(sGetDHCPTuningDebugRespT);

  return XST_SUCCESS;
}


#define BITSET(flag,bit)  flag = (((unsigned int) flag) | ((unsigned int) 1u << bit));
#define BITCLR(flag,bit)  flag = (((unsigned int) flag) & (~((unsigned int) 1u << bit)));

/*
 *  Get the current monitor fault logs
 */
static int GetCurrentLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  u8 num_entries;
  sLogDataEntryT log_entry;
  u8 *byte_buffer_ptr;
  /* u8 uPaddingIndex; */
  u8 j, index;
  size_t i;
  int status = 0;

  sGetCurrentLogsReqT *Command = (sGetCurrentLogsReqT *) pCommand;
  sGetCurrentLogsRespT *Response = (sGetCurrentLogsRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetCurrentLogsReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetCurrentLogsRespT);

  /*
   * Get all the populated log entries from the monitoring device.
   * If the device has less than NUM_LOG_ENTRIES populated
   * in the log table / fifo, return 0xffff for all fields of that
   * entry.
   */

  byte_buffer_ptr = (u8 *) Response->uCurrentLogEntries;
  /* fill the whole block of the cmd payload with 0xff */
  for (i = 0; i < (NUM_LOG_ENTRIES * sizeof(sLogDataEntryT)); i++){
    byte_buffer_ptr[i] = 0xff;
  }

  /*
   * Start with the premise that all log entry reads will be successful. Since the log
   * entries are populated with the latest entry at the last valid index, it may be a
   * potential issue if the success bit is zero initially since we would not be able to
   * discern between an i2c retrieval error or a valid empty entry for the last entry.
   * Thus for valid empty entries, the success bit will be set to '1'.
   */
  Response->uLogEntrySuccess = 0xffff;  /* will clear the failed entries later */

  /* get the number of entries in the log - max = NUM_LOG_ENTRIES */
  status = get_num_current_log_entries(&num_entries);
  if (XST_SUCCESS != status){
    Response->uLogEntrySuccess = 0;
    return XST_SUCCESS;   /* return success in order for skarab to reply with failed status word */
  }

  /* now fill in the entries according to nun_entries */
  for (index = 0; index < num_entries; index++){
    status = get_current_log_entry(index, &log_entry);
    if (XST_SUCCESS != status){
      BITCLR(Response->uLogEntrySuccess, index);
    } else {
      Response->uCurrentLogEntries[index].uPageSpecific = log_entry.uPageSpecific;
      Response->uCurrentLogEntries[index].uFaultType = log_entry.uFaultType;
      Response->uCurrentLogEntries[index].uPage = log_entry.uPage;
      Response->uCurrentLogEntries[index].uFaultValue = log_entry.uFaultValue;
      Response->uCurrentLogEntries[index].uValueScale = log_entry.uValueScale;
      for (j=0; j<2;j++){
        Response->uCurrentLogEntries[index].uSeconds[j] = log_entry.uSeconds[j];
#if 0
        Response->uCurrentLogEntries[index].uMilliSeconds[j] = log_entry.uMilliSeconds[j];
        Response->uCurrentLogEntries[index].uDays[j] = log_entry.uDays[j];
#endif
      }
      BITSET(Response->uLogEntrySuccess, index);
    }
  }

#if 0
  for (uPaddingIndex = 0; uPaddingIndex < 1; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }
#endif

  return XST_SUCCESS;
}




/* TODO:  this was a copy and paste exercise from the above current handler - find another way to do this
 *        since the functionality of the functions are basically identical! */

static int GetVoltageLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  u8 num_entries;
  sLogDataEntryT log_entry;
  u8 *byte_buffer_ptr;
  /* u8 uPaddingIndex; */
  u8 j, index;
  size_t i;
  int status = 0;

  sGetVoltageLogsReqT *Command = (sGetVoltageLogsReqT *) pCommand;
  sGetVoltageLogsRespT *Response = (sGetVoltageLogsRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetVoltageLogsReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetVoltageLogsRespT);

  /*
   * Get all the populated log entries from the monitoring device.
   * If the device has less than NUM_LOG_ENTRIES populated
   * in the log table / fifo, return 0xffff for all fields of that
   * entry.
   */

  byte_buffer_ptr = (u8 *) Response->uVoltageLogEntries;
  /* fill the whole block of the cmd payload with 0xff */
  for (i = 0; i < (NUM_LOG_ENTRIES * sizeof(sLogDataEntryT)); i++){
    byte_buffer_ptr[i] = 0xff;
  }

  /*
   * Start with the premise that all log entry reads will be successful. Since the log
   * entries are populated with the latest entry at the last valid index, it may be a
   * potential issue if the success bit is zero initially since we would not be able to
   * discern between an i2c retrieval error or a valid empty entry for the last entry.
   * Thus for valid empty entries, the success bit will be set to '1'.
   */
  Response->uLogEntrySuccess = 0xffff;  /* will clear the failed entries later */

  /* get the number of entries in the log - max = NUM_LOG_ENTRIES */
  status = get_num_voltage_log_entries(&num_entries);
  if (XST_SUCCESS != status){
    Response->uLogEntrySuccess = 0;
    return XST_SUCCESS;   /* return success in order for skarab to reply with failed status word set above */
  }

  /* now fill in the entries according to nun_entries */
  for (index = 0; index < num_entries; index++){
    status = get_voltage_log_entry(index, &log_entry);
#if 0
    /*FIXME: simulating an error*/
    status = index == 1 ? XST_FAILURE : status;
#endif
    if (XST_SUCCESS != status){
      BITCLR(Response->uLogEntrySuccess, index);
    } else {
      Response->uVoltageLogEntries[index].uPageSpecific = log_entry.uPageSpecific;
      Response->uVoltageLogEntries[index].uFaultType = log_entry.uFaultType;
      Response->uVoltageLogEntries[index].uPage = log_entry.uPage;
      Response->uVoltageLogEntries[index].uFaultValue = log_entry.uFaultValue;
      Response->uVoltageLogEntries[index].uValueScale = log_entry.uValueScale;
      for (j=0; j<2;j++){
        Response->uVoltageLogEntries[index].uSeconds[j] = log_entry.uSeconds[j];
#if 0
        Response->uVoltageLogEntries[index].uMilliSeconds[j] = log_entry.uMilliSeconds[j];
        Response->uVoltageLogEntries[index].uDays[j] = log_entry.uDays[j];
#endif
      }
      BITSET(Response->uLogEntrySuccess, index);
    }
  }

#if 0
  for (uPaddingIndex = 0; uPaddingIndex < 1; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }
#endif

  return XST_SUCCESS;
}


static int GetFanControllerLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  int i, j, status = 0;
  u8 *byte_buffer_ptr;
  //sTempLogDataEntryT log_entry;
  sFanCtrlrLogDataEntryT log_entry;
  u16 offset;

  sGetFanControllerLogsReqT *Command = (sGetFanControllerLogsReqT *) pCommand;
  sGetFanControllerLogsRespT *Response = (sGetFanControllerLogsRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetFanControllerLogsReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetFanControllerLogsRespT);

  byte_buffer_ptr = (u8 *) Response->uFanCtrlrLogEntries;
  /* fill the whole block of the cmd payload with 0xff */
  for (i = 0; i < (NUM_FANCTRLR_LOG_ENTRIES * sizeof(sFanCtrlrLogDataEntryT)); i++){
    byte_buffer_ptr[i] = 0xff;
  }


  /* there is a max of 15 fault log entries on the MAX31785 device */
  for (j = 0; j < NUM_FANCTRLR_LOG_ENTRIES; j++){
    status += get_fanctrlr_log_entry_next(&log_entry);
    /*
     * either returns a valid index between 0 and 14, or returns an
     * 0xff if that location is empty.
     */
    offset = log_entry.uFaultLogIndex;
    if ((offset >= 0) && (offset <= 14)){
      /* fill in the response packet at the appropriate offset */
      /* TODO [p-10] could probably do a memcpy or for-loop byte copy here... */
      Response->uFanCtrlrLogEntries[offset].uFaultLogIndex = log_entry.uFaultLogIndex;
      Response->uFanCtrlrLogEntries[offset].uFaultLogCount = log_entry.uFaultLogCount;
      Response->uFanCtrlrLogEntries[offset].uStatusWord = log_entry.uStatusWord;
      Response->uFanCtrlrLogEntries[offset].uStatusVout_17_18 = log_entry.uStatusVout_17_18;
      Response->uFanCtrlrLogEntries[offset].uStatusVout_19_20 = log_entry.uStatusVout_19_20;
      Response->uFanCtrlrLogEntries[offset].uStatusVout_21_22 = log_entry.uStatusVout_21_22;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_6_7 = log_entry.uStatusMfr_6_7;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_8_9 = log_entry.uStatusMfr_8_9;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_10_11 = log_entry.uStatusMfr_10_11;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_12_13 = log_entry.uStatusMfr_12_13;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_14_15 = log_entry.uStatusMfr_14_15;
      Response->uFanCtrlrLogEntries[offset].uStatusMfr_16_none = log_entry.uStatusMfr_16_none;
      Response->uFanCtrlrLogEntries[offset].uStatusFans_0_1 = log_entry.uStatusFans_0_1;
      Response->uFanCtrlrLogEntries[offset].uStatusFans_2_3 = log_entry.uStatusFans_2_3;
      Response->uFanCtrlrLogEntries[offset].uStatusFans_4_5 = log_entry.uStatusFans_4_5;
    }
    else {
      continue;
      /* skip -> will be filled with earlier 0xff's */
    }
  }

  Response->uCompleteSuccess = (status == 0 ? 1 : 0);

  for (i = 0; i < 3; i++){
    Response->uPadding[i] = 0;
  }

  return XST_SUCCESS;
}


static int ClearFanControllerLogsHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
  int status, i;

  sClearFanControllerLogsReqT *Command = (sClearFanControllerLogsReqT *) pCommand;
  sClearFanControllerLogsRespT *Response = (sClearFanControllerLogsRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sClearFanControllerLogsReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sClearFanControllerLogsRespT);

  status = clear_fanctrlr_logs();

  Response->uSuccess = (status == XST_SUCCESS ? 1 : 0);

  for (i = 0; i < 8; i++){
    Response->uPadding[i] = 0;
  }



  return XST_SUCCESS;
}


static int ResetDHCPStateMachine(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  struct sIFObject *ifptr;
  u8 n;

  sDHCPResetStateMachineReqT *Command = (sDHCPResetStateMachineReqT *) pCommand;
  sDHCPResetStateMachineRespT *Response = (sDHCPResetStateMachineRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sDHCPResetStateMachineReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sDHCPResetStateMachineRespT);
  Response->uLinkId = Command->uLinkId;

  n = get_num_interfaces();

  /* check that the request link id is within bounds */
  if (Command->uLinkId >= n){
    /* check it here since the i/f handle lookup asserts this condition, which we don't want for user facing APIs */
    Response->uResetError = 1;  /* link non-existent */
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] cmd 0x%04x - no such link id: %d\r\n", Command->Header.uCommandType, Command->uLinkId);
  } else {
    ifptr = lookup_if_handle_by_id(Command->uLinkId);

    if (ifptr->uIFLinkStatus == LINK_DOWN){
      log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] cmd 0x%04x - link %d is down\r\n", Command->Header.uCommandType, Command->uLinkId);
      Response->uResetError = 2; /* link down */
    } else {
      vDHCPStateMachineReset(ifptr);
      uDHCPSetStateMachineEnable(ifptr, SM_TRUE);

      Response->uResetError = 0;  /* no errors */
    }
  }

  return XST_SUCCESS;
}


static int MulticastLeaveGroup(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  u8 status;

  sMulticastLeaveGroupReqT *Command = (sMulticastLeaveGroupReqT *) pCommand;
  sMulticastLeaveGroupRespT *Response = (sMulticastLeaveGroupRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sMulticastLeaveGroupReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sMulticastLeaveGroupRespT);

  Response->uLinkId = Command->uLinkId;

  status = uIGMPLeaveGroup(Command->uLinkId);

  if (XST_SUCCESS == status){
    Response->uSuccess = 1;
  } else {
    Response->uSuccess = 0;
  }

  return XST_SUCCESS;

}


static int GetDHCPMonitorTimeout(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  sGetDHCPMonitorTimeoutReqT *Command = (sGetDHCPMonitorTimeoutReqT*) pCommand;
  sGetDHCPMonitorTimeoutRespT *Response = (sGetDHCPMonitorTimeoutRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetDHCPMonitorTimeoutReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetDHCPMonitorTimeoutRespT);

  Response->uDHCPMonitorTimeout = GlobalDHCPMonitorTimeout;

  return XST_SUCCESS;
}


static int GetMicroblazeUptime(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  u32 uptime;

  sGetMicroblazeUptimeReqT *Command = (sGetMicroblazeUptimeReqT*) pCommand;
  sGetMicroblazeUptimeRespT *Response = (sGetMicroblazeUptimeRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetMicroblazeUptimeReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetMicroblazeUptimeRespT);

  uptime = get_microblaze_uptime_seconds();

  Response->uMicroblazeUptimeHigh = (uptime >> 16) & 0xffff;
  Response->uMicroblazeUptimeLow = (uptime & 0xffff);

  return XST_SUCCESS;
}


/*
  En    Upd   Wr
  0      0    0     No fan control operation but does restore the default values
  1      0    0     Invalid choice - cannot enable fan control without setting setpoints. This is to ensure valid
                    control setpoints
  1      1    0     Set the setpoints and enable fan control
  0      1    0     Only set the setpoints, leave other setting unchanged
  x      x    1     Write the current settings to FLASH

*/

static int FPGAFanControllerUpdateHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  int status;
  u8 uPaddingIndex;

  sFPGAFanControllerUpdateReqT *Command = (sFPGAFanControllerUpdateReqT *) pCommand;
  sFPGAFanControllerUpdateRespT *Response = (sFPGAFanControllerUpdateRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sFPGAFanControllerUpdateReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sFPGAFanControllerUpdateRespT);

  for (uPaddingIndex = 0; uPaddingIndex < 8; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }

  /* interlock - ensure that the setpoints are being set when we're enabling fan control */
  /* NOTE: this does not safeguard us against invalid setpoints - this is a TODO in
           the fanctrlr_set_lut_points() function - but it at least makes the user
           aware of the dependency if the flag has not been set */
  /* ie you can set setpoints without enabling, but not other way round */
  if ((1 == Command->uEnableFanControl) && (Command->uUpdateSetpoints != 1 )){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] cannot enable fan control without setting setpoints\r\n");
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field will flag the error */
  }

  status = fanctrlr_configure_mux_switch();
  if (XST_FAILURE == status){
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field will flag the error */
  }

  status = fanctrlr_restore_defaults();
  if (XST_FAILURE == status){
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field will flag the error */
  }


  if (1 == Command->uUpdateSetpoints){
    status = fanctrlr_set_lut_points(Command->uSetpoints);
    if (XST_FAILURE == status){
      goto failure;
    }
  }


  if (1 == Command->uEnableFanControl){
    status = fanctrlr_setup_registers();
    if (XST_FAILURE == status){
      goto failure;
    }


    status = fanctrlr_enable_auto_fan_control();
    if (XST_FAILURE == status){
      goto failure;
    }
  }


  if (1 == Command->uWriteToFlash){
    status = fanctrlr_store_defaults_to_flash();
    if (XST_FAILURE == status){
      goto failure;
    }
  }

  /* status: send 0 for success, >=1 for error */
  Response->uError = 0;
  return XST_SUCCESS;

failure:
  /* try to restore defaults */
  fanctrlr_restore_defaults();
  Response->uError = 1;
  return XST_SUCCESS;   /* return success in order for calling function
                           to send reply - uError field (>=1) will flag
                           the error */
}




static int GetFPGAFanControllerLUTHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  u8 uPaddingIndex;
  u16 *lut_ptr = NULL;
  int i;
  int status;

  sGetFPGAFanControllerLUTReqT *Command = (sGetFPGAFanControllerLUTReqT *) pCommand;
  sGetFPGAFanControllerLUTRespT *Response = (sGetFPGAFanControllerLUTRespT *) uResponsePacketPtr;

  if (uCommandLength < sizeof(sGetFPGAFanControllerLUTReqT)){
    return XST_FAILURE;
  }

  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  *uResponseLength = sizeof(sGetFPGAFanControllerLUTRespT);

  for (uPaddingIndex = 0; uPaddingIndex < 4; uPaddingIndex++){
    Response->uPadding[uPaddingIndex] = 0;
  }


  status = fanctrlr_configure_mux_switch();
  if (XST_FAILURE == status){
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field will flag the error */
  }


  status = fanctrlr_restore_defaults();
  if (XST_FAILURE == status){
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field will flag the error */
  }


  lut_ptr = fanctrlr_get_lut_points();
  if (NULL == lut_ptr){
    Response->uError = 1;
    return XST_SUCCESS;   /* return success in order for calling function
                             to send reply - uError field (>=1) will flag
                             the error */
  }

  /* copy into response payload */
  for (i=0; i<16; i++){
    Response->uSetpointData[i] = lut_ptr[i];
  }

  Response->uError = 0;
  return XST_SUCCESS;
}
