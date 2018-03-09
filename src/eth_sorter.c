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

#include "eth_sorter.h"
#include "net_utils.h"
#include "dhcp.h"
#include "one_wire.h"

//=================================================================================
//	CalculateIPChecksum
//--------------------------------------------------------------------------------
//	This method calculates the IPv4 header checksum.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uChecksum	IN		Initial checksum value
//	uLength		IN		Number of bytes
//	pHeaderPtr	IN		Packet header pointer
//
//	Return
//	------
//	Checksum
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
	//	uChecksum += (u32) pHeaderPtr[1];

	//uChecksum = (uChecksum & 0x0FFFFu) + (uChecksum >> 16);
	//uChecksum = (uChecksum & 0x0FFFFu) + (uChecksum >> 16);

	while (uChecksum>>16)
	{
		uChecksum = (uChecksum & 0xFFFF)+(uChecksum >> 16);
	}

	return uChecksum;
}

//=================================================================================
//	CheckIPV4Header
//--------------------------------------------------------------------------------
//	This method validates the IPv4 packet header.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uIPAddress			IN	IP address of FPGA fabric
//	uSubnet				IN	Subnet mask
//	uPacketLength		IN	Packet length
//	pIPHeaderPointer	IN	Pointer to header
//
//	Return
//	------
//	XST_SUCCESS if successful
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
		xil_printf("IP Length problem\r\n");
		return XST_FAILURE;
	}

	struct sIPV4Header *IPHeader = (struct sIPV4Header *) pIPHeaderPointer;

	uDestinationIPAddress = (IPHeader->uDestinationIPHigh << 16) | IPHeader->uDestinationIPLow;

	//xil_printf("VERS: %x TOS: %x LEN: %x ID: %x FLGS: %x TTL: %x PROT: %x CHK: %x SRC: %x %x DEST: %x %x\r\n", IPHeader->uVersion, IPHeader->uTypeOfService, IPHeader->uTotalLength, IPHeader->uIdentification, IPHeader->uFlagsFragment, IPHeader->uTimeToLive, IPHeader->uProtocol, IPHeader->uChecksum,  IPHeader->uSourceIPHigh, IPHeader->uSourceIPLow, IPHeader->uDestinationIPHigh, IPHeader->uDestinationIPLow );

	if (uPacketLength < IPHeader->uTotalLength)
	{
		xil_printf("IP Total length problem PKT: %x\r\n", uPacketLength);

		//xil_printf("VERS: %x TOS: %x LEN: %x ID: %x FLGS: %x TTL: %x PROT: %x CHK: %x SRC: %x %x DEST: %x %x\r\n", IPHeader->uVersion, IPHeader->uTypeOfService, IPHeader->uTotalLength, IPHeader->uIdentification, IPHeader->uFlagsFragment, IPHeader->uTimeToLive, IPHeader->uProtocol, IPHeader->uChecksum,  IPHeader->uSourceIPHigh, IPHeader->uSourceIPLow, IPHeader->uDestinationIPHigh, IPHeader->uDestinationIPLow );

    /*  TODO FIXME uReceiveBuffer now a 2D array
		for (uIndex = 0; uIndex < 256; uIndex++)
			xil_printf("%x DATA: %x\r\n", uIndex, uReceiveBuffer[uIndex]);
    */

		return XST_FAILURE;
	}

	// IP version
	if ((IPHeader->uVersion & 0xF0) != 0x40)
	{
		xil_printf("IP Version problem\r\n");
		return XST_FAILURE;
	}

	// IP checksum - long version
	uIPHeaderLength = 4 * (IPHeader->uVersion & 0x0F);
	uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) pIPHeaderPointer);

	if (uChecksum != 0xFFFFu)
	{
		xil_printf("IP Checksum problem\r\n");
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

#if 0
//=================================================================================
//	CheckICMPHeader
//--------------------------------------------------------------------------------
//	This method validates the ICMP packet header.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uPacketLength	IN	Packet length
//	pICMPHeaderPtr	IN	Pointer to header
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int CheckICMPHeader(u32 uPacketLength, u8 * pICMPHeaderPtr)
{
	u32 uChecksum;

	if (uPacketLength < sizeof(struct sICMPHeader))
	{
		xil_printf("ICMP Length problem\r\n");
		return XST_FAILURE;
	}

	struct sICMPHeader * ICMPHeader = (struct sICMPHeader *) pICMPHeaderPtr;

	if (ICMPHeader->uType != 8)
	{
		xil_printf("ICMP Type problem\r\n");
		return XST_FAILURE;
	}

	if (ICMPHeader->uCode != 0)
	{
		xil_printf("ICMP Code problem\r\n");
		return XST_FAILURE;
	}

	uChecksum = CalculateIPChecksum(0, uPacketLength / 2, (u16 *) pICMPHeaderPtr);

	if (uChecksum != 0xFFFFu)
	{
		xil_printf("ICMP Checksum problem\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

//=================================================================================
//	ICMPHandler
//--------------------------------------------------------------------------------
//	This method creates the ICMP response packet.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	ID of selected Ethernet interface
//	pReceivedICMPPacket	IN	Pointer to ICMP packet
//	uReceivedLength		IN	Length of ICMP packet
//	pTransmitBuffer		OUT	Pointer to transmit buffer where response will be created
//	uResponseLength		OUT	Length of response packet
//
//	Return
//	------
//	None
//=================================================================================
void ICMPHandler(u8 uId, u8 *pReceivedICMPPacket, u32 uReceivedLength, u8 *pTransmitBuffer, u32 * uResponseLength)
{
	struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) pTransmitBuffer;
	u8 * uIPResponseHeader = pTransmitBuffer + sizeof(sEthernetHeaderT);
	struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPResponseHeader;
	u8 * uICMPResponseHeader = uIPResponseHeader + sizeof(sIPV4HeaderT);
	struct sICMPHeader * ICMPHeaderResponse = (struct sICMPHeader *) uICMPResponseHeader;
	struct sICMPHeader * ICMPHeaderRequest = (struct sICMPHeader *) pReceivedICMPPacket;
	u8 * uICMPResponsePayload = uICMPResponseHeader + sizeof(sICMPHeaderT);
	u8 * uICMPRequestPayload = pReceivedICMPPacket + sizeof(sICMPHeaderT);
	u32 uChecksum;
	u32 uIPHeaderLength = sizeof(sIPV4HeaderT);
	u32 uICMPLength = uReceivedLength;
	u32 uIndex;
	u32 uTempResponseLength;

	EthernetHeader->uDestMacHigh = uResponseMacHigh;
	EthernetHeader->uDestMacMid = uResponseMacMid;
	EthernetHeader->uDestMacLow = uResponseMacLow;

	EthernetHeader->uSourceMacHigh = uEthernetFabricMacHigh[uId];
	EthernetHeader->uSourceMacMid = uEthernetFabricMacMid[uId];
	EthernetHeader->uSourceMacLow = uEthernetFabricMacLow[uId];

	EthernetHeader->uEthernetType = ETHERNET_TYPE_IPV4;

	IPHeader->uVersion = 0x40 | sizeof(sIPV4HeaderT) / 4;

	IPHeader->uTypeOfService = 0;
	IPHeader->uTotalLength = uReceivedLength + sizeof(sIPV4HeaderT);

	IPHeader->uFlagsFragment = 0x4000;
	IPHeader->uIdentification = uIPIdentification[uId]++;

	IPHeader->uChecksum = 0;

	IPHeader->uProtocol = IP_PROTOCOL_ICMP;
	IPHeader->uTimeToLive = 0x80;

	IPHeader->uSourceIPHigh = (uEthernetFabricIPAddress[uId] >> 16) & 0xFFFF;
	IPHeader->uSourceIPLow = uEthernetFabricIPAddress[uId] & 0xFFFF;
	IPHeader->uDestinationIPHigh = (uResponseIPAddr >> 16) & 0xFFFF;
	IPHeader->uDestinationIPLow = uResponseIPAddr & 0xFFFF;

	uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) IPHeader);
	IPHeader->uChecksum = ~uChecksum;

	ICMPHeaderResponse->uType = 0;
	ICMPHeaderResponse->uCode = 0;
	ICMPHeaderResponse->uChecksum = 0;
	ICMPHeaderResponse->uIdentifier = ICMPHeaderRequest->uIdentifier;
	ICMPHeaderResponse->uSequenceNumber = ICMPHeaderRequest->uSequenceNumber;

	if (uReceivedLength > sizeof(sICMPHeaderT))
	{
		// Copy across the data portion of the ICMP request
		for (uIndex = 0; uIndex < (uReceivedLength - sizeof(sICMPHeaderT)); uIndex++)
		{
			* uICMPResponsePayload = *uICMPRequestPayload;
			uICMPResponsePayload++;
			uICMPRequestPayload++;
		}
	}

	uChecksum = CalculateIPChecksum(0, uICMPLength / 2, (u16 *) ICMPHeaderResponse);
	ICMPHeaderResponse->uChecksum = ~ uChecksum;

	uTempResponseLength = sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + uReceivedLength;

	// Round up to nearest number of 64-bit words
	while ((uTempResponseLength % 8) != 0)
	{
		* uICMPResponsePayload = 0x0;
		uICMPResponsePayload++;
		uTempResponseLength++;
	}

	* uResponseLength = uTempResponseLength;

}
#endif

//=================================================================================
//	ExtractIPV4FieldsAndGetPayloadPointer
//--------------------------------------------------------------------------------
//	This method extracts the useful information from the IPV4 header and returns
//	a pointer to the start of the IP payload.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pIPHeaderPointer	IN	Pointer to IP header
//	uIPPayloadLength	OUT	Length of IP payload
//	uResponseIPAddr		OUT	Source IP address (use as destination in response)
//	uProtocol			OUT	IP packet protocol
//	uTOS				OUT IP Type Of Service
//
//	Return
//	------
//	Pointer to IP payload
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
//	ExtractIPV4FieldsAndGetUDPPointer
//--------------------------------------------------------------------------------
//	This method extracts the useful information from the IPV4 header and returns
//	a pointer to the start of the IP payload.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pIPHeaderPointer	IN		Pointer to IP header
//	uIPPayloadLength	IN		Length of IP payload
//	pUdpHeaderPointer	IN		Pointer to UDP header
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int CheckUdpHeader(u8 *pIPHeaderPointer, u32 uIPPayloadLength, u8 *pUdpHeaderPointer)
{
	u32 uIPHeaderLength;
	u32 uChecksum;
	u32 uUdpLength;

	if (uIPPayloadLength < sizeof(struct sUDPHeader))
	{
		xil_printf("UDP Length problem\r\n");
		return XST_FAILURE;
	}

	struct sUDPHeader *UDPHeader = (struct sUDPHeader *) pUdpHeaderPointer;
	if (uIPPayloadLength < UDPHeader->uTotalLength)
	{
		xil_printf("UDP Total length problem\r\n");
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
		xil_printf("UDP Checksum problem\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

//=================================================================================
//	ExtractUdpFieldsAndGetPayloadPointer
//--------------------------------------------------------------------------------
//	This method extracts the useful information from the UDP header and returns
//	a pointer to the start of the UDP payload.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pUdpHeaderPointer	IN	Pointer to UDP header
//	uPayloadLength		OUT	Length of UDP payload
//	uSourcePort			OUT	Source port address
//	uDestinationPort	OUT	Destination port address
//
//	Return
//	------
//	Pointer to UDP payload
//=================================================================================
u8 *ExtractUdpFieldsAndGetPayloadPointer(u8 *pUdpHeaderPointer,u32 *uPayloadLength, u32 *uSourcePort, u32 *uDestinationPort)
{
	struct sUDPHeader *UdpHeader = (struct sUDPHeader *) pUdpHeaderPointer;

	*uPayloadLength = UdpHeader->uTotalLength - sizeof(struct sUDPHeader);
	*uSourcePort = UdpHeader->uSourcePort;
	*uDestinationPort = UdpHeader->uDestinationPort;

	return (pUdpHeaderPointer + sizeof(struct sUDPHeader));
}

//=================================================================================
//	CommandSorter
//--------------------------------------------------------------------------------
//	This method calls the difference command handlers depending on command received.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//  uId       IN    ID of the interface the command was received on
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Location to put response packet
//	uResponseLength			OUT	Length of response packet payload
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int CommandSorter(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength, struct sIFObject *pIFObj)
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
		else if (Command->uCommandType == DHCP_TUNING_DEBUG)
			return(DHCPTuningDebugCommandHandler(pIFObj, pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
		else{
			xil_printf("Invalid Opcode Detected!\r\n");
			return(InvalidOpcodeHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
			//return XST_FAILURE;
		}
			
	}
	else
		xil_printf("Invalid Opcode Detected: Out of Range!\r\n");
		return(InvalidOpcodeHandler(pCommand, uCommandLength, uResponsePacketPtr, uResponseLength));
		//return XST_FAILURE;

	return XST_SUCCESS;

}

//=================================================================================
//	CheckCommandPacket
//--------------------------------------------------------------------------------
//	This method does some basic checks on the command received.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand		IN	Pointer to command header
//	uCommandLength	IN	Length of command
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int CheckCommandPacket(u8 * pCommand, u32 uCommandLength)
{
	//static u16 uPreviousSequenceNumber = 0;
	sCommandHeaderT *Command = (sCommandHeaderT *) pCommand;

	if (uCommandLength < sizeof(sCommandHeaderT))
	{
		xil_printf("Length problem!\r\n");
		return XST_FAILURE;
	}

	// Command must be odd (response even)
	if ((Command->uCommandType & 1) != 1)
	{
		xil_printf("Command type problem!\r\n");
		return XST_FAILURE;
	}

	// Out of range
	if (Command->uCommandType > HIGHEST_DEFINED_COMMAND)
	{
		xil_printf("Command range problem!\r\n");
		return XST_FAILURE;
	}

	// Sequence number should not be same as previous sequence number
	//if (Command->uSequenceNumber == uPreviousSequenceNumber)
	//{
	//	xil_printf("Sequence number problem!\r\n");
	//	return XST_FAILURE;
	//}

	// Update sequence number
	//uPreviousSequenceNumber = Command->uSequenceNumber;

	return XST_SUCCESS;
}

//=================================================================================
//	CreateResponsePacket
//--------------------------------------------------------------------------------
//	This method creates the Ethernet, IP and UDP headers for the response packet.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	ID of selected Ethernet interface
//	uResponsePacketPtr	OUT	Pointer to construct response packet
//	uResponseLength		IN	Length of response payload
//
//	Return
//	------
//	None
//=================================================================================
void CreateResponsePacket(u8 uId, u8 * uResponsePacketPtr, u32 uResponseLength)
{
	struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) uResponsePacketPtr;
	u8 * uIPResponseHeader = uResponsePacketPtr + sizeof(sEthernetHeaderT);
	struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPResponseHeader;
	u8 * uUdpResponseHeader = uIPResponseHeader + sizeof(sIPV4HeaderT);
	struct sUDPHeader * UdpHeader = (struct sUDPHeader *) uUdpResponseHeader;
	u32 uChecksum;
	u32 uIPHeaderLength = sizeof(sIPV4HeaderT);
	u32 uUdpLength = uResponseLength + sizeof(sUDPHeaderT);

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

	IPHeader->uSourceIPHigh = (uEthernetFabricIPAddress[uId] >> 16) & 0xFFFF;
	IPHeader->uSourceIPLow = uEthernetFabricIPAddress[uId] & 0xFFFF;
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
//	WriteRegCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the WRITE_REG command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

	//xil_printf("BRD: %x ADDR: %x DATA: %x\r\n", Command->uBoardReg, Command->uRegAddress, uReg);

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
//	ReadRegCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the READ_REG command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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
//	WriteWishboneCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the WRITE_WISHBONE command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int WriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sWriteWishboneReqT *Command = (sWriteWishboneReqT *) pCommand;
	sWriteWishboneRespT *Response = (sWriteWishboneRespT *) uResponsePacketPtr;
	u32 uAddress;
	u32 uWriteData;
	u8 uPaddingIndex;

  xil_printf("Wishbone write...\r\n");
	if (uCommandLength < sizeof(sWriteWishboneReqT))
		return XST_FAILURE;

	uAddress = (Command->uAddressHigh << 16) | (Command->uAddressLow);
	uWriteData = (Command->uWriteDataHigh << 16) | (Command->uWriteDataLow);

	// Execute the command
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress,uWriteData);

	Response->Header.uCommandType = Command->Header.uCommandType + 1;
	Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
	Response->uAddressHigh = Command->uAddressHigh;
	Response->uAddressLow = Command->uAddressLow;
	Response->uWriteDataHigh = Command->uWriteDataHigh;
	Response->uWriteDataLow = Command->uWriteDataLow;

	for (uPaddingIndex = 0; uPaddingIndex < 5; uPaddingIndex++)
		Response->uPadding[uPaddingIndex] = 0;

	*uResponseLength = sizeof(sWriteWishboneRespT);

	return XST_SUCCESS;
}

//=================================================================================
//	ReadWishboneCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the READ_WISHBONE command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sReadWishboneReqT *Command = (sReadWishboneReqT *) pCommand;
	sReadWishboneRespT *Response = (sReadWishboneRespT *) uResponsePacketPtr;
	u32 uAddress;
	u32 uReadData;
	u8 uPaddingIndex;

	if (uCommandLength < sizeof(sReadWishboneReqT))
		return XST_FAILURE;

	uAddress = (Command->uAddressHigh << 16) | (Command->uAddressLow);

	// Execute the command
	uReadData = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddress);

	Response->Header.uCommandType = Command->Header.uCommandType + 1;
	Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;
	Response->uAddressHigh = Command->uAddressHigh;
	Response->uAddressLow = Command->uAddressLow;
	Response->uReadDataHigh = (uReadData >> 16) & 0xFFFF;
	Response->uReadDataLow = uReadData & 0xFFFF;

	for (uPaddingIndex = 0; uPaddingIndex < 5; uPaddingIndex++)
		Response->uPadding[uPaddingIndex] = 0;

	*uResponseLength = sizeof(sReadWishboneRespT);

	return XST_SUCCESS;
}

//=================================================================================
//	WriteI2CCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the WRITE_I2C command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int WriteI2CCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sWriteI2CReqT *Command = (sWriteI2CReqT *) pCommand;
	sWriteI2CRespT *Response = (sWriteI2CRespT *) uResponsePacketPtr;
	int iStatus;
	u8 uIndex;
	u32 uTimeout;

	if (uCommandLength < sizeof(sWriteI2CReqT))
		return XST_FAILURE;

#ifdef DEBUG_PRINT
	//xil_printf("ID: %x SLAVE ADDRESS: %x NUM BYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uNumBytes);

	//for (uIndex = 0; uIndex < Command->uNumBytes; uIndex++)
	//	xil_printf("INDEX: %x DATA: %x\r\n", uIndex, Command->uWriteBytes[uIndex]);
#endif


	if ((uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT)&&((uQSFPMezzanineLocation + 1) == Command->uId))
	{
		// If we are accessing the QSFP module, set the QSFP monitoring state back to start
		uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;

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
	for (uIndex = 0; uIndex < 32; uIndex++)
		Response->uWriteBytes[uIndex] = Command->uWriteBytes[uIndex];

	if (iStatus == XST_SUCCESS)
		Response->uWriteSuccess = 1;
	else
		Response->uWriteSuccess = 0;

	Response->uPadding[0] = 0;

	*uResponseLength = sizeof(sWriteI2CRespT);

	return XST_SUCCESS;
}

//=================================================================================
//	ReadI2CCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the READ_I2C command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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
	//xil_printf("ID: %x SLAVE ADDRESS: %x NUM BYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uNumBytes);
#endif

	if ((uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT)&&((uQSFPMezzanineLocation + 1) == Command->uId))
	{
		// If we are accessing the QSFP module, set the QSFP monitoring state back to start
		uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;

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
		xil_printf("INDEX: %x READ DATA: %x\r\n", uIndex, Response->uReadBytes[uIndex]);
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
//	SdramReconfigureCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the SDRAM_RECONFIGURE command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//  uId       IN    ID of the interface the command was received on
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	if (Command->uDoSdramAsyncRead == 0)
	{
		xil_printf("OUT: %x CLR SDR: %x FIN: %x ABT: %x RBT: %x\r\n", Command->uOutputMode, Command->uClearSdram, Command->uFinishedWritingToSdram, Command->uAboutToBootFromSdram, Command->uDoReboot);
		xil_printf("RST: %x CLR: %x ENBL: %x DO RD: %x\r\n", Command->uResetSdramReadAddress, Command->uClearEthernetStatistics, Command->uEnableDebugSdramReadMode, Command->uDoSdramAsyncRead);
	}
#endif

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
#ifdef DEBUG_PRINT
    xil_printf("Received SDRAM reconfig command on %s-i/f with id %d\r\n", uId == 0 ? "1gbe" : "40gbe", uId);
    xil_printf("Setting board register 0x%.4x to 0x%.4x\r\n", C_WR_BRD_CTL_STAT_1_ADDR, uMuxSelect);
		xil_printf("About to send IGMP leave messages.\r\n");
#endif

    /* The clear sdram command is usually called before programming. Thus, in anticipation of a new sdram image being sent */
    /* to the skarab, unsubscribe from all igmp groups. */

    // Check which Ethernet interfaces are part of IGMP groups
    // and send leave messages immediately
    for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
    {
      if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
      {
        uIGMPState[uIndex] = IGMP_STATE_LEAVING;
        uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
        uCurrentIGMPMessage[uIndex] = 0x0;
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
		xil_printf("About to reboot. Sending IGMP leave messages now.\r\n");

		// IcapeControllerInSystemReconfiguration();
		uDoReboot = REBOOT_REQUESTED;

		// Check which Ethernet interfaces are part of IGMP groups
		// and send leave messages immediately
		for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
		{
			if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
			{
				uIGMPState[uIndex] = IGMP_STATE_LEAVING;
				uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
				uCurrentIGMPMessage[uIndex] = 0x0;
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
		//xil_printf("READ DATA: %x\r\n", uReg);
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
//	ReadFlashWordsCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the READ_FLASH_WORDS command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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
	//xil_printf("NUM WRDS: %x\r\n", Command->uNumWords);
	for (uIndex = 0; uIndex < Command->uNumWords; uIndex++)
	{
		Response->uReadWords[uIndex] = ReadWord(uAddress);
		//xil_printf("INDX: %x READ: %x\r\n", uIndex, Response->uReadWords[uIndex]);
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
//	ProgramFlashWordsCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the PROGRAM_FLASH_WORDS command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ProgramFlashWordsCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sProgramFlashWordsReqT *Command = (sProgramFlashWordsReqT *) pCommand;
	sProgramFlashWordsRespT *Response = (sProgramFlashWordsRespT *) uResponsePacketPtr;
	u32 uIndex;
	u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;
	int iStatus = XST_SUCCESS;

#ifdef DEBUG_PRINT
	xil_printf("PRGRM FLASH: %x\r\n", uAddress);
#endif

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
//	EraseFlashBlockCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the ERASE_FLASH_BLOCK command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	xil_printf("BLK ADDR: %x\r\n", uBlockAddress);
#endif

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
//	ReadSpiPageCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the READ_SPI_PAGE command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ReadSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sReadSpiPageReqT *Command = (sReadSpiPageReqT *) pCommand;
	sReadSpiPageRespT *Response = (sReadSpiPageRespT *) uResponsePacketPtr;
	int iStatus;
	u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;

	if (uCommandLength < sizeof(sReadSpiPageReqT))
		return XST_FAILURE;

#ifdef DEBUG_PRINT
	xil_printf("SPI READ ADDR: %x NUM BYTES: %x\r\n", uAddress, Command->uNumBytes);
#endif

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
//	ProgramSpiPageCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the PROGRAM_SPI_PAGE command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ProgramSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sProgramSpiPageReqT *Command = (sProgramSpiPageReqT *) pCommand;
	sProgramSpiPageRespT *Response = (sProgramSpiPageRespT *) uResponsePacketPtr;
	int iStatus;
	u32 uAddress = (Command->uAddressHigh << 16) | Command->uAddressLow;

	if (uCommandLength < sizeof(sProgramSpiPageReqT))
		return XST_FAILURE;

#ifdef DEBUG_PRINT
	xil_printf("ADDR: %x NUM BYTES: %x\r\n", uAddress, Command->uNumBytes);
#endif

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
//	EraseSpiSectorCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the ERASE_SPI_SECTOR command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int EraseSpiSectorCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sEraseSpiSectorReqT *Command = (sEraseSpiSectorReqT *) pCommand;
	sEraseSpiSectorRespT *Response = (sEraseSpiSectorRespT *) uResponsePacketPtr;
	int iStatus;
	u32 uSectorAddress = (Command->uSectorAddressHigh << 16) | Command->uSectorAddressLow;
	u8 uPaddingIndex;

	xil_printf("SPI SECT ERASE ADDR: %x\r\n", uSectorAddress);

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
//	OneWireReadRomCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the ONE_WIRE_READ_ROM command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	for (uIndex = 0; uIndex < 8; uIndex++)
		xil_printf("INDX: %x ROM: %x\r\n", uIndex, Response->uRom[uIndex]);
#endif

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
//	OneWireDS2433WriteMemCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the ONE_WIRE_DS2433_WRITE_MEM command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	xil_printf("SKP ROM: %x, NUM BYTES: %x TA1: %x TA2: %x ID: %x", Command->uSkipRomAddress, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);
#endif

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
//	OneWireDS2433ReadMemCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the ONE_WIRE_DS2433_READ_MEM command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	xil_printf("SKIP: %x NUMBYTES: %x TA1: %x TA2: %x ID: %x\r\n", Command->uSkipRomAddress, Command->uNumBytes, Command->uTA1, Command->uTA2, Command->uOneWirePort);
#endif

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
//	DebugConfigureEthernetCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the DEBUG_CONFIGURE_ETHERNET command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	xil_printf("ID: %x MAC: %x %x %x ENABLE: %x\r\n", Command->uId, Command->uFabricMacHigh, Command->uFabricMacMid, Command->uFabricMacLow, Command->uEnableFabricInterface);
	xil_printf("PORT: %x GTWAY: %x %x IP: %x MCSTIP: %x MCSTIPMSK: %x\r\n", Command->uFabricPortAddress, Command->uGatewayIPAddressHigh, Command->uGatewayIPAddressLow, uFabricIPAddress, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);
#endif

	// Execute the command

	// CURRENTLY ONLY USED WHEN TESTING IN LOOPBACK AND DHCP DOESN'T COMPLETE
	// SetFabricSourceMACAddress(Command->uId, Command->uFabricMacHigh, (Command->uFabricMacMid << 16)|(Command->uFabricMacLow)); SET BASED ON SERIAL NUMBER
	// SetFabricSourcePortAddress(Command->uId, Command->uFabricPortAddress); ONLY DEFAULT VALUE USED
	//SetFabricGatewayARPCacheAddress(Command->uId, (Command->uGatewayIPAddressLow & 0xFF)); NOT NEEDED IN LOOPBACK
	SetFabricSourceIPAddress(Command->uId, uFabricIPAddress);
	//SetMultiCastIPAddress(Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask); NOT NEEDED IN LOOPBACK
	EnableFabricInterface(Command->uId, Command->uEnableFabricInterface);

	uDHCPState[Command->uId] = DHCP_STATE_COMPLETE;

	uEthernetSubnet[Command->uId] = (uFabricIPAddress & 0xFFFFFF00);

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
	uEthernetFabricIPAddress[Command->uId] = uFabricIPAddress;
	//uEthernetGatewayIPAddress[Command->uId] = (Command->uGatewayIPAddressHigh << 16) | Command->uGatewayIPAddressLow;
	//uEthernetFabricPortAddress[Command->uId] = Command->uFabricPortAddress;
	//uEthernetFabricMultiCastIPAddress[Command->uId] = uFabricMultiCastIPAddress;
	//uEthernetFabricMultiCastIPAddressMask[Command->uId] = uFabricMultiCastIPAddressMask;


	return XST_SUCCESS;
}

//=================================================================================
//	DebugAddARPCacheEntryCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the DEBUG_ADD_ARP_CACHE_ENTRY command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int DebugAddARPCacheEntryCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sDebugAddARPCacheEntryReqT *Command = (sDebugAddARPCacheEntryReqT *) pCommand;
	sDebugAddARPCacheEntryRespT *Response = (sDebugAddARPCacheEntryRespT *) uResponsePacketPtr;
	u8 uPaddingIndex;

	if (uCommandLength < sizeof(sDebugAddARPCacheEntryReqT))
		return XST_FAILURE;

#ifdef DEBUG_PRINT
	xil_printf("ID: %x INDX: %x MAC: %x %x %x\r\n", Command->uId, Command->uIPAddressLower8Bits, Command->uMacHigh, Command->uMacMid, Command->uMacLow);
#endif

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
//	GetEmbeddedSoftwareVersionCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the GET_EMBEDDED_SOFTWARE_VERS command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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
//	PMBusReadI2CBytesCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the PMBUS_READ_I2C command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int PMBusReadI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sPMBusReadI2CBytesReqT *Command = (sPMBusReadI2CBytesReqT *) pCommand;
	sPMBusReadI2CBytesRespT *Response = (sPMBusReadI2CBytesRespT *) uResponsePacketPtr;
	int iStatus;

	if (uCommandLength < sizeof(sPMBusReadI2CBytesReqT))
		return XST_FAILURE;

#ifdef DEBUG_PRINT
	//xil_printf("ID: %x SLV: %x CMD: %x NMBYTES: %x\r\n", Command->uId, Command->uSlaveAddress, Command->uCommandCode, Command->uNumBytes);
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
//	ConfigureMulticastCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the CONFIGURE_MULTICAST command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

#ifdef DEBUG_PRINT
	xil_printf("ID: %x MC IP: %x MC MASK: %x \r\n", Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);
#endif

	// Execute the command
	SetMultiCastIPAddress(Command->uId, uFabricMultiCastIPAddress, uFabricMultiCastIPAddressMask);

	uEthernetFabricMultiCastIPAddress[Command->uId] = uFabricMultiCastIPAddress;
	uEthernetFabricMultiCastIPAddressMask[Command->uId] = uFabricMultiCastIPAddressMask;

	// Enable sending of Multicast packets on this interface
	uIGMPState[Command->uId] = IGMP_STATE_JOINED_GROUP;
	uIGMPSendMessage[Command->uId] = IGMP_SEND_MESSAGE;
	uCurrentIGMPMessage[Command->uId] = 0x0;

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

//=================================================================================
//	CheckArpRequest
//--------------------------------------------------------------------------------
//	This method checks whether valid ARP request and whether ARP request is for us.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	ID of network interface that received ARP request
//	uFabricIPAddress	IN	Our IP address
//	uPktLen				IN	Length of packet
//	pArpPacket			IN	Pointer to packet
//
//	Return
//	------
//	XST_SUCCESS if successful match
//=================================================================================
int CheckArpRequest(u8 uId, u32 uFabricIPAddress, u32 uPktLen, u8 *pArpPacket)
{
	u32 uSenderIPAddress;

	if (uPktLen < sizeof(sArpPacketT))
	{
		xil_printf("ARP Length problem\r\n");
		return XST_FAILURE;
	}

	struct sArpPacket *Arp = (struct sArpPacket *) pArpPacket;

	if (Arp->uHardwareType != 1 || Arp->uProtocolType != ETHERNET_TYPE_IPV4)
	{
		xil_printf("ARP Type problem\r\n");
		return XST_FAILURE;
	}

	if (Arp->uHardwareLength != 6 || Arp->uProtocolLength != 4)
	{
		xil_printf("ARP Hardware length problem\r\n");
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
			xil_printf("IP ADDRESS CONFLICT DETECTED! RESTARTING DHCP ON ID: %x\r\n", uId);
			uDHCPState[uId] = DHCP_STATE_DISCOVER;
			uDHCPRetryTimer[uId] = DHCP_RETRY_ENABLED;
		}
		else
		{
			// It was a response so add it to the ARP cache
#ifdef DEBUG_PRINT
			xil_printf("ARP ENTRY ID: %x IP: %x %x MAC: %x %x %x\r\n", uId, Arp->uSenderIpHigh, Arp->uSenderIPLow, Arp->uSenderMacHigh, Arp->uSenderMacMid, Arp->uSenderMacLow);
#endif

			ProgramARPCacheEntry(uId, (Arp->uSenderIPLow & 0xFF), Arp->uSenderMacHigh, ((Arp->uSenderMacMid << 16) | Arp->uSenderMacLow));
		}

		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

//=================================================================================
//	ArpHandler
//--------------------------------------------------------------------------------
//	This method creates the ARP response packet.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId				IN	ID of selected Ethernet interface
//	uType			IN	1 for request, 2 for response
//	pReceivedArp	IN	Pointer to received ARP packet
//	pTransmitBuffer	OUT	Pointer to transmit buffer where ARP response is created
//	uResponseLength	OUT	Size of ARP packet created
//	uRequestedIPAddress	IN	For a request, the IP address that trying to get MAC address for
//
//	Return
//	------
//	None
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

#if 0
//=================================================================================
//	CreateDHCPDiscoverPacketOptions
//--------------------------------------------------------------------------------
//	This method creates the set of options needed for a DHCP DISCOVER packet.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uDHCPOptionsLength 	OUT	Length of DHCP options
//
//	Return
//	------
//	None
//=================================================================================
void CreateDHCPDiscoverPacketOptions(u8 uId, u8 *pTransmitBuffer, u32 * uDHCPOptionsLength)
{
	u8 * uDHCPOptions = pTransmitBuffer +  sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT) + sizeof(sDHCPHeaderT);
	u8 uIndex;
  u8 uTempDigit;

	// Create a transaction ID that is unique for each interface
	// May need to include serial number to make globally unique
	uDHCPTransactionID[uId] = uDHCPTransactionID[uId] + NUM_ETHERNET_INTERFACES;

	// Bytes are swapped so this is why the order is swapped

	// Option 53, option 55, option 12, terminate
	*uDHCPOptions++ = 1; // Length is 1
	*uDHCPOptions++ = DHCP_MESSAGE_OPTION;

	*uDHCPOptions++ = DHCP_PARAMETER_REQUEST_OPTION;
	*uDHCPOptions++ = DHCP_MESSAGE_DISCOVER;

	*uDHCPOptions++ = DHCP_PARAMETER_ROUTER;
	*uDHCPOptions++ = 1; // Only requesting 1 parameter

  /* Add host name to DHCP packet - Option 12 - rvw-03-2017 */

	*uDHCPOptions++ = 15;  //hostname option length = 'skarab' (6) + serial# (6) + dash (1) + interface id (2) => e.g. skarab020202-01
	*uDHCPOptions++ = DHCP_HOST_NAME_OPTION;

  *uDHCPOptions++ = 'k';
  *uDHCPOptions++ = 's';

  *uDHCPOptions++ = 'r';
  *uDHCPOptions++ = 'a';

  *uDHCPOptions++ = 'b';
  *uDHCPOptions++ = 'a';

  uTempDigit = ((uEthernetFabricMacMid[uId] >> 8) & 0xff) % 0x10;  /* lower digit of upper octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId] >> 8) & 0xff) / 0x10;  /* upper digit of upper octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId]) & 0xff) % 0x10;  /* lower digit of lower octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId]) & 0xff) / 0x10;  /* upper digit of lower octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacLow[uId] >> 8) & 0xff) % 0x10;  /* lower digit of upper octet of mac-low */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacLow[uId] >> 8) & 0xff) / 0x10;  /* upper digit of upper octet of mac-low */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  *uDHCPOptions++ = (uId / 10) + 48;  /* tens digit of interface id*/
  *uDHCPOptions++ = '-';

  *uDHCPOptions++ = DHCP_PAD_OPTION;
  *uDHCPOptions++ = (uId % 10) + 48;  /* unit digit of interface id*/

	// End the options list
	*uDHCPOptions++ = DHCP_PAD_OPTION;
	*uDHCPOptions++ = DHCP_END_OPTION;

	// Pad to nearest 64 bit boundary
	for (uIndex = 0; uIndex < 4; uIndex++)
		*uDHCPOptions++ = DHCP_PAD_OPTION;

	*uDHCPOptionsLength = 30;
}

//=================================================================================
//	CreateDHCPRequestPacketOptions
//--------------------------------------------------------------------------------
//	This method creates the set of options needed for a DHCP REQUEST packet.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uDHCPOptionsLength 	OUT	Length of DHCP options
//	uRequestedIPAddress	IN	Requested IP address
//	uServerIPAddress	IN	Server IP address to issue request to
//
//	Return
//	------
//	None
//=================================================================================
void CreateDHCPRequestPacketOptions(u8 uId, u8 *pTransmitBuffer, u32 * uDHCPOptionsLength, u32 uRequestedIPAddress, u32 uServerIPAddress)
{
	u8 * uDHCPOptions = pTransmitBuffer +  sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT) + sizeof(sDHCPHeaderT);
	u8 uIndex;
  u8 uTempDigit;

	// Bytes are swapped so this is why the order is swapped

	// Option 53, 50, 54, 12, terminate
	*uDHCPOptions++ = 1; // Length is 1
	*uDHCPOptions++ = DHCP_MESSAGE_OPTION;

	*uDHCPOptions++ = DHCP_REQUESTED_IP_OPTION;
	*uDHCPOptions++ = DHCP_MESSAGE_REQUEST;

	*uDHCPOptions++ = (uRequestedIPAddress >> 24) & 0xFF;
	*uDHCPOptions++ = 4;

	*uDHCPOptions++ = (uRequestedIPAddress >> 8) & 0xFF;
	*uDHCPOptions++ = (uRequestedIPAddress >> 16) & 0xFF;

	*uDHCPOptions++ = DHCP_SERVER_OPTION;
	*uDHCPOptions++ = uRequestedIPAddress & 0xFF;

	*uDHCPOptions++ = (uServerIPAddress >> 24) & 0xFF;
	*uDHCPOptions++ = 4;

	*uDHCPOptions++ = (uServerIPAddress >> 8) & 0xFF;
	*uDHCPOptions++ = (uServerIPAddress >> 16) & 0xFF;

	*uDHCPOptions++ = DHCP_PAD_OPTION;
	*uDHCPOptions++ = uServerIPAddress & 0xFF;

  /* Add host name to DHCP packet - Option 12 - rvw-03-2017 */

	*uDHCPOptions++ = 15;  //hostname option length = 'skarab' (6) + serial# (6) + dash (1) + interface id (2) => e.g. skarab020202-01
	*uDHCPOptions++ = DHCP_HOST_NAME_OPTION;

  *uDHCPOptions++ = 'k';
  *uDHCPOptions++ = 's';

  *uDHCPOptions++ = 'r';
  *uDHCPOptions++ = 'a';

  *uDHCPOptions++ = 'b';
  *uDHCPOptions++ = 'a';

  uTempDigit = ((uEthernetFabricMacMid[uId] >> 8) & 0xff) % 0x10;  /* lower digit of upper octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId] >> 8) & 0xff) / 0x10;  /* upper digit of upper octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId]) & 0xff) % 0x10;  /* lower digit of lower octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacMid[uId]) & 0xff) / 0x10;  /* upper digit of lower octet of mac-mid */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacLow[uId] >> 8) & 0xff) % 0x10;  /* lower digit of upper octet of mac-low */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  uTempDigit = ((uEthernetFabricMacLow[uId] >> 8) & 0xff) / 0x10;  /* upper digit of upper octet of mac-low */
  *uDHCPOptions++ = uTempDigit > 9 ? ((uTempDigit - 10) + 0x41) : (uTempDigit + 0x30);

  *uDHCPOptions++ = (uId / 10) + 48;  /* tens digit of interface id*/
  *uDHCPOptions++ = '-';

  *uDHCPOptions++ = DHCP_PAD_OPTION;
  *uDHCPOptions++ = (uId % 10) + 48;  /* unit digit of interface id*/

	// End the options list
  *uDHCPOptions++ = DHCP_PAD_OPTION;
	*uDHCPOptions++ = DHCP_END_OPTION;

	// Pad to nearest 64 bit boundary
	for (uIndex = 0; uIndex < 2; uIndex++)
		*uDHCPOptions++ = DHCP_PAD_OPTION;

	* uDHCPOptionsLength = 38;
}

//=================================================================================
//	CreateDHCPPacket
//--------------------------------------------------------------------------------
//	This method creates a DHCP packet with no options on the selected Ethernet interface.
//	It assumes that the required options have already been populated in the transmit
//	buffer.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uResponseLength		OUT	Length of packet created
//	uDHCPOptionsLength 	IN	Length of DHCP options
//
//	Return
//	------
//	None
//=================================================================================
void CreateDHCPPacket(u8 uId, u8 *pTransmitBuffer, u32 * uResponseLength, u32 uDHCPOptionsLength)
{
	struct sEthernetHeader *EthernetHeader = (struct sEthernetHeader *) pTransmitBuffer;
	u8 * uIPV4Header = pTransmitBuffer + sizeof(sEthernetHeaderT);
	struct sIPV4Header *IPHeader = (struct sIPV4Header *) uIPV4Header;
	u8 * uUdpHeader = uIPV4Header + sizeof(sIPV4HeaderT);
	struct sUDPHeader * UdpHeader = (struct sUDPHeader *) uUdpHeader;
	u8 * uDHCPHeader = uUdpHeader + sizeof(sUDPHeaderT);
	struct sDHCPHeader * DHCPHeader = (struct sDHCPHeader *) uDHCPHeader;

	u8 uIndex;
	u32 uChecksum;
	u32 uIPHeaderLength = sizeof(sIPV4HeaderT);
	u32 uUdpLength = uDHCPOptionsLength + sizeof(sDHCPHeaderT) + sizeof(sUDPHeaderT);

	EthernetHeader->uDestMacHigh = 0xFFFF;
	EthernetHeader->uDestMacMid = 0xFFFF;
	EthernetHeader->uDestMacLow = 0xFFFF;

	EthernetHeader->uSourceMacHigh = uEthernetFabricMacHigh[uId];
	EthernetHeader->uSourceMacMid = uEthernetFabricMacMid[uId];
	EthernetHeader->uSourceMacLow = uEthernetFabricMacLow[uId];

	EthernetHeader->uEthernetType = ETHERNET_TYPE_IPV4;

	IPHeader->uVersion = 0x40 | sizeof(sIPV4HeaderT) / 4;

	IPHeader->uTypeOfService = 0;
	IPHeader->uTotalLength = uDHCPOptionsLength + sizeof(sDHCPHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT);

	IPHeader->uFlagsFragment = 0x4000;
	IPHeader->uIdentification = uIPIdentification[uId]++;

	IPHeader->uChecksum = 0;

	IPHeader->uProtocol = IP_PROTOCOL_UDP;
	IPHeader->uTimeToLive = 0x80;

	IPHeader->uSourceIPHigh = 0x0000;
	IPHeader->uSourceIPLow = 0x0000;
	IPHeader->uDestinationIPHigh = 0xFFFF;
	IPHeader->uDestinationIPLow = 0xFFFF;

	uChecksum = CalculateIPChecksum(0, uIPHeaderLength / 2, (u16 *) IPHeader);
	IPHeader->uChecksum = ~uChecksum;

	UdpHeader->uTotalLength = uDHCPOptionsLength + sizeof(sDHCPHeaderT) + sizeof(sUDPHeaderT);
	UdpHeader->uChecksum = 0;
	UdpHeader->uSourcePort = DHCP_CLIENT_UDP_PORT;
	UdpHeader->uDestinationPort = DHCP_SERVER_UDP_PORT;

	uChecksum = (IPHeader->uProtocol & 0x0FFu) + ((IPHeader->uTotalLength - uIPHeaderLength)
			& 0x0FFFFu);

	uChecksum += IPHeader->uSourceIPHigh + IPHeader->uSourceIPLow;
	uChecksum += IPHeader->uDestinationIPHigh + IPHeader->uDestinationIPLow;

	DHCPHeader->uOpCode = 1; // Request
	DHCPHeader->uHardwareType = 1; // Ethernet
	DHCPHeader->uHardwareAddressLength = 6;
	DHCPHeader->uHops = 0;
	DHCPHeader->uXidHigh = ((uDHCPTransactionID[uId] >> 16) & 0xFFFF);
	DHCPHeader->uXidLow = (uDHCPTransactionID[uId] & 0xFFFF);
	DHCPHeader->uSeconds = 0;
	DHCPHeader->uFlags = 0x8000; // Broadcast, don't know IP address yet

	DHCPHeader->uClientIPAddressHigh = 0x0;
	DHCPHeader->uClientIPAddressLow = 0x0;
	DHCPHeader->uYourIPAddressHigh = 0x0;
	DHCPHeader->uYourIPAddressLow = 0x0;
	DHCPHeader->uServerIPAddressHigh = 0x0; // Not sure if this should by 0 in the DHCPREQUEST as well
	DHCPHeader->uServerIPAddressLow = 0x0;
	DHCPHeader->uGatewayIPAddressHigh = 0x0;
	DHCPHeader->uGatewayIPAddressLow = 0x0;

	DHCPHeader->uClientHardwareAddress[0] = uEthernetFabricMacHigh[uId];
	DHCPHeader->uClientHardwareAddress[1] = uEthernetFabricMacMid[uId];
	DHCPHeader->uClientHardwareAddress[2] = uEthernetFabricMacLow[uId];

	for (uIndex = 3; uIndex < 8; uIndex++)
		DHCPHeader->uClientHardwareAddress[uIndex] = 0x0;

	for (uIndex = 0; uIndex < 96; uIndex++)
		DHCPHeader->uBootPLegacy[uIndex] = 0x0;

	DHCPHeader->uMagicCookieHigh = ((DHCP_MAGIC_COOKIE >> 16) & 0xFFFF);
	DHCPHeader->uMagicCookieLow = (DHCP_MAGIC_COOKIE & 0xFFFF);

	uChecksum = CalculateIPChecksum(uChecksum, uUdpLength / 2, (u16 *) UdpHeader);
	UdpHeader->uChecksum = ~uChecksum;

	* uResponseLength = uDHCPOptionsLength + sizeof(sDHCPHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT) + sizeof(sEthernetHeaderT);

}

//=================================================================================
//	CheckDHCPHeader
//--------------------------------------------------------------------------------
//	This method does basic checks of the DHCP header.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId				IN	Selected Ethernet interface
//	uDHCPPacketLength	IN	Length of DHCP portion of packet
//	pDHCPHeaderPointer	IN	Pointer to buffer containing DHCP header
//
//	Return
//	------
//	XST_SUCCESS if DHCP header is valid and for us
//=================================================================================
int CheckDHCPHeader(u8 uId, u32 uDHCPPacketLength, u8 * pDHCPHeaderPointer)
{
	if (uDHCPPacketLength < sizeof(struct sDHCPHeader))
	{
		xil_printf("DHCP Length problem\r\n");
		return XST_FAILURE;
	}

	struct sDHCPHeader * DHCPHeader = (struct sDHCPHeader *) pDHCPHeaderPointer;

	if (DHCPHeader->uOpCode != 2)
	{
		xil_printf("DHCP Opcode problem\r\n");
		return XST_FAILURE;
	}

	if (DHCPHeader->uHardwareType != 1)
	{
		xil_printf("DHCP Hardware type problem\r\n");
		return XST_FAILURE;
	}

	if (DHCPHeader->uHardwareAddressLength != 6)
	{
		xil_printf("DHCP Hardware address length problem\r\n");
		return XST_FAILURE;
	}

	if (DHCPHeader->uClientHardwareAddress[0] != uEthernetFabricMacHigh[uId])
		return XST_FAILURE;

	if (DHCPHeader->uClientHardwareAddress[1] != uEthernetFabricMacMid[uId])
		return XST_FAILURE;

	if (DHCPHeader->uClientHardwareAddress[2] != uEthernetFabricMacLow[uId])
		return XST_FAILURE;

	if (DHCPHeader->uMagicCookieHigh != ((DHCP_MAGIC_COOKIE >> 16) & 0xFFFF))
		return XST_FAILURE;

	if (DHCPHeader->uMagicCookieLow != (DHCP_MAGIC_COOKIE & 0xFFFF))
		return XST_FAILURE;

	return XST_SUCCESS;
}

//=================================================================================
//	DHCPHandler
//--------------------------------------------------------------------------------
//	This method handles the received DHCP packets and creates the appropriate responses
//	where necessary.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pReceivedDHCPPacket	IN	Received DHCP portion of the packet
//	uReceivedLength		IN	Length of received DHCP portion
//	pTransmitBuffer		OUT	Location of transmit buffer (to create DHCP request)
//	uResponseLength		OUT	Length of DHCP request
//
//	Return
//	------
//	XST_SUCCESS if a response must be generated
//=================================================================================
int DHCPHandler(u8 uId, u8 *pReceivedDHCPPacket, u32 uReceivedLength, u8 *pTransmitBuffer, u32 * uResponseLength)
{
	struct sDHCPHeader * DHCPHeader = (struct sDHCPHeader *) pReceivedDHCPPacket;

	u8 * uDHCPOptions = pReceivedDHCPPacket + sizeof(sDHCPHeaderT);
	u8 uOptionLength;
	u8 uDHCPOptionNow;
	u8 uDHCPMessageOption = DHCP_MESSAGE_INFORM;
	u32 uDHCPRouterOption = 0xFFFFFFFF;
	u32 uDHCPServerOption = 0xFFFFFFFF;
	u32 uDHCPRequestedIPAddress;
	u32 uDHCPOptionLength;

#define SIZE 312    //required minimum number of octets a DHCP client must be prepared to receive - RFC2131 (par. 2,pg. 10)
  u8 uDHCPOptionByteSwap[SIZE];
	//u8 uDHCPOptionByteSwap[64]; // COULD CAUSE PROBLEMS IF DHCP SERVER CREATES LARGE OPTIONS!!!

	u16 uIndex;
	/* u8 uFoundTerminate = 0x0; */

	// Need to swap the bytes as process received DHCP packet
	uIndex = 0;

#ifdef TRACE_PRINT
	xil_printf("DHCP[%02x]: byte swap buffer: \r\n", uId);
#endif

	do
	{
		uDHCPOptionByteSwap[uIndex + 1] = *uDHCPOptions++;
		uDHCPOptionByteSwap[uIndex] = *uDHCPOptions++;
#ifdef TRACE_PRINT
		xil_printf("%s", ((uIndex != 0) & (uIndex % 16 == 0)) ? "\r\n" : "");
		xil_printf("0x%02x 0x%02x ", uDHCPOptionByteSwap[uIndex], uDHCPOptionByteSwap[uIndex + 1]);
#endif
		uIndex = uIndex + 2;
	}while(uIndex < SIZE);

#ifdef TRACE_PRINT
	xil_printf("\r\n");
#endif

	uDHCPRequestedIPAddress = (DHCPHeader->uYourIPAddressHigh << 16) | DHCPHeader->uYourIPAddressLow;

	// Need to process the options to get required information
	uIndex = 0;

  while (1)   /* this is not a real endless loop, it will end once dhcp option processing done */
  {
    if (uIndex >= SIZE){
#ifdef TRACE_PRINT
      xil_printf("DHCP[%02x] index %u exceeds buffer size %u\r\n", uId, uIndex, SIZE);
#endif
      return XST_FAILURE;             /* reached end of buffer without finding dhcp end option */
    }

    uDHCPOptionNow = uDHCPOptionByteSwap[uIndex];
    if (uDHCPOptionNow == DHCP_END_OPTION){   /* found the end option, leave the option processing loop */
#ifdef TRACE_PRINT
      xil_printf("DHCP[%02x] end option found at %u\r\n", uId, uIndex);
#endif
      break;
    }

    if ((uIndex + 1) >= SIZE){      /* check that we're not about to read past the edge of our buffer */
#ifdef TRACE_PRINT
      xil_printf("DHCP[%02x] index %u exceeds buffer size %u\r\n", uId, uIndex + 1, SIZE);
#endif
      return XST_FAILURE;
    }

    uOptionLength = uDHCPOptionByteSwap[uIndex + 1];

    if ((uIndex + uOptionLength) >= SIZE){      /* check that we're not about to read past the edge of our buffer */
#ifdef TRACE_PRINT
      xil_printf("DHCP[%02x] index %u exceeds buffer size %u\r\n", uId, uIndex + uOptionLength, SIZE);
#endif
      return XST_FAILURE;
    }

#ifdef TRACE_PRINT
    xil_printf("DHCP[%02x] option %u with length %u at index %u\r\n", uId, uDHCPOptionNow, uOptionLength, uIndex);
#endif

    /* parse the dhcp options */
    switch(uDHCPOptionNow)
    {
      case DHCP_MESSAGE_OPTION:
        uDHCPMessageOption = uDHCPOptionByteSwap[uIndex + 2];
        uIndex = uIndex + uOptionLength + 2;
        break;

      case DHCP_ROUTER_OPTION:
        uDHCPRouterOption = (uDHCPOptionByteSwap[uIndex + 2] << 24) | (uDHCPOptionByteSwap[uIndex + 3] << 16) | (uDHCPOptionByteSwap[uIndex + 4] << 8) | (uDHCPOptionByteSwap[uIndex + 5]);
        uIndex = uIndex + uOptionLength + 2;
        break;

      case DHCP_SERVER_OPTION:
        uDHCPServerOption = (uDHCPOptionByteSwap[uIndex + 2] << 24) | (uDHCPOptionByteSwap[uIndex + 3] << 16) | (uDHCPOptionByteSwap[uIndex + 4] << 8) | (uDHCPOptionByteSwap[uIndex + 5]);
        uIndex = uIndex + uOptionLength + 2;
        break;

      default:        /* skip past all other options not listed above */
#ifdef TRACE_PRINT
        xil_printf("DHCP[%02x] ignoring option %u\r\n", uId, uDHCPOptionNow);
#endif
        uIndex = uIndex + uOptionLength + 2;
        break;
    }
  }

#if 0
	while ((uFoundTerminate == 0x0)&&(uIndex < 64))
	{
		if (uDHCPOptionByteSwap[uIndex] == DHCP_MESSAGE_OPTION)
		{
			uOptionLength = uDHCPOptionByteSwap[uIndex + 1];
			uDHCPMessageOption = uDHCPOptionByteSwap[uIndex + 2];
			uIndex = uIndex + uOptionLength + 2;
		}
		else if (uDHCPOptionByteSwap[uIndex] == DHCP_ROUTER_OPTION)
		{
			uOptionLength = uDHCPOptionByteSwap[uIndex + 1];
			uDHCPRouterOption = (uDHCPOptionByteSwap[uIndex + 2] << 24) | (uDHCPOptionByteSwap[uIndex + 3] << 16) | (uDHCPOptionByteSwap[uIndex + 4] << 8) | (uDHCPOptionByteSwap[uIndex + 5]);
			uIndex = uIndex + uOptionLength + 2;
		}
		else if (uDHCPOptionByteSwap[uIndex] == DHCP_SERVER_OPTION)
		{
			uOptionLength = uDHCPOptionByteSwap[uIndex + 1];
			uDHCPServerOption = (uDHCPOptionByteSwap[uIndex + 2] << 24) | (uDHCPOptionByteSwap[uIndex + 3] << 16) | (uDHCPOptionByteSwap[uIndex + 4] << 8) | (uDHCPOptionByteSwap[uIndex + 5]);
			uIndex = uIndex + uOptionLength + 2;
		}
		else if (uDHCPOptionByteSwap[uIndex] == DHCP_END_OPTION)
		{
			uFoundTerminate = 1;
			uIndex++;
		}
		else
		{
			// For other options, get the length and skip past
			uOptionLength = uDHCPOptionByteSwap[uIndex + 1];
			uIndex = uIndex + uOptionLength + 2;
		}
	}

	if (uIndex == 64)
	// Read end of the options and didn't get a terminate
		return XST_FAILURE;
#endif

#undef SIZE

	if (uDHCPMessageOption == DHCP_MESSAGE_OFFER)
	{
		uDHCPState[uId] = DHCP_STATE_REQUEST;

		// Now need to create request
		CreateDHCPRequestPacketOptions(uId, pTransmitBuffer, & uDHCPOptionLength, uDHCPRequestedIPAddress, uDHCPServerOption);
		CreateDHCPPacket(uId, pTransmitBuffer, uResponseLength, uDHCPOptionLength);
		return XST_SUCCESS;
	}
	else if (uDHCPMessageOption == DHCP_MESSAGE_ACK)
	{
#ifdef DEBUG_PRINT
		xil_printf("DHCP [%02x] Setting IP address to: %u.%u.%u.%u\r\n", uId, ((uDHCPRequestedIPAddress >> 24) & 0xFF), 
                                                                          ((uDHCPRequestedIPAddress >> 16) & 0xFF),
                                                                          ((uDHCPRequestedIPAddress >> 8) & 0xFF),
                                                                          (uDHCPRequestedIPAddress & 0xFF));
#endif

		uDHCPState[uId] = DHCP_STATE_COMPLETE;

	    uEthernetFabricIPAddress[uId] = uDHCPRequestedIPAddress;

		SetFabricSourceIPAddress(uId, uEthernetFabricIPAddress[uId]);

		uEthernetGatewayIPAddress[uId] = uDHCPRouterOption;

		uEthernetSubnet[uId] = (uDHCPRequestedIPAddress & 0xFFFFFF00);

		SetFabricGatewayARPCacheAddress(uId, (uDHCPRouterOption & 0xFF));

		// Add an entry for own IP address in ARP cache table for loopback testing
#ifdef DEBUG_PRINT
		xil_printf("ARP ENTRY ID: %x IP: %x MAC: %x %x %x\r\n", uId, uDHCPRequestedIPAddress, uEthernetFabricMacHigh[uId], uEthernetFabricMacMid[uId], uEthernetFabricMacLow[uId]);
#endif

		ProgramARPCacheEntry(uId, (uDHCPRequestedIPAddress & 0xFF), uEthernetFabricMacHigh[uId], ((uEthernetFabricMacMid[uId] << 16) | uEthernetFabricMacLow[uId]));

		// Enable ARP requests now that have IP address
		uEnableArpRequests[uId] = ARP_REQUESTS_ENABLE;

#ifdef DEBUG_PRINT
		xil_printf("Enabling ETH MAC FPGA fabric %x interface...\r\n", uId);
#endif
		EnableFabricInterface(uId, 0x1);

		// Leave the front panel LEDs on if 1GBE has completed DHCP
		//if (uId == 0x0)
		//{
			//uFrontPanelLeds = LED_ON;
			//WriteBoardRegister(C_WR_FRONT_PANEL_STAT_LED_ADDR, 0xFF);
		WriteBoardRegister(C_WR_FRONT_PANEL_STAT_LED_ADDR, uFrontPanelLedsValue);

		//}

		// No response needed so return XST_FAILURE (even though everything OK)
		return XST_FAILURE;

	}
	else if (uDHCPMessageOption == DHCP_MESSAGE_NAK)
	{
		// If NAK then set state back to DISCOVER
		uDHCPState[uId] = DHCP_STATE_DISCOVER;
		uDHCPRetryTimer[uId] = DHCP_RETRY_ENABLED;

		return XST_FAILURE;
	}
	else
		return XST_FAILURE;

}
#endif

//=================================================================================
//	CreateIGMPPacket
//--------------------------------------------------------------------------------
//	This method is used to create the two types of IGMP messages to the router/switch.
//	Instead of handling query messages, membership reports packets are generated
//	every 60 seconds.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location of transmit buffer (to create IGMP packet)
//	uResponseLength		OUT	Length of IGMP packet
//	uMessageType		IN	Whether it is a membership report or leave message
//	uGroupAddress		IN	Address for IGMP message
//
//	Return
//	------
//	None
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

	u8 uIndex;
	u32 uChecksum;
	u32 uIPHeaderLength = sizeof(sIPV4HeaderT) + sizeof(sIPV4HeaderOptionsT);
	u32 uIGMPHeaderLength = sizeof(sIGMPHeaderT);

	u32 uTempGroupAddress;

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

	IPHeader->uSourceIPHigh = ((uEthernetFabricIPAddress[uId] >> 16) & 0xFFFF);
	IPHeader->uSourceIPLow = (uEthernetFabricIPAddress[uId] & 0xFFFF);

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
//	DebugLoopbackTestCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the DEBUG_LOOPBACK_TEST command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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

	if (uCommandLength < sizeof(sDebugLoopbackTestReqT))
		return XST_FAILURE;

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
	IPHeader->uSourceIPHigh = (uEthernetFabricIPAddress[Command->uId] >> 16) & 0xFFFF;
	IPHeader->uSourceIPLow = uEthernetFabricIPAddress[Command->uId] & 0xFFFF;
	IPHeader->uDestinationIPHigh = (uEthernetFabricIPAddress[Command->uId] >> 16) & 0xFFFF;
	IPHeader->uDestinationIPLow = uEthernetFabricIPAddress[Command->uId] & 0xFFFF;

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
//	QSFPResetAndProgramCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the QSFP_RESET_AND_PROG command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int QSFPResetAndProgramCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength)
{
	sQSFPResetAndProgramReqT *Command = (sQSFPResetAndProgramReqT *) pCommand;
	sQSFPResetAndProgramRespT *Response = (sQSFPResetAndProgramRespT *) uResponsePacketPtr;
	u8 uPaddingIndex;
	u32 uReg = uWriteBoardShadowRegs[C_WR_MEZZANINE_CTL_ADDR >> 2];

	if (uCommandLength < sizeof(sQSFPResetAndProgramReqT))
		return XST_FAILURE;

	if (Command->uReset == 0x1)
	{
#ifdef DEBUG_PRINT
		xil_printf("Resetting QSFP+ Mezzanine.\r\n");
#endif
		uReg = uReg | (QSFP_MEZZANINE_RESET << uQSFPMezzanineLocation);
		WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, uReg);
		Delay(100);
		uReg = uReg & (~ (QSFP_MEZZANINE_RESET << uQSFPMezzanineLocation));
		WriteBoardRegister(C_WR_MEZZANINE_CTL_ADDR, uReg);

		uQSFPMezzanineCurrentTxLed = 0x0;
		uQSFPMezzanineCurrentRxLed = 0x0;
		uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;
		uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

		uQSFPCtrlReg = QSFP0_RESET | QSFP1_RESET | QSFP2_RESET | QSFP3_RESET;
		WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);

		if (Command->uProgram == 0x1)
		{
#ifdef DEBUG_PRINT
		xil_printf("QSFP+ Mezzanine entering bootloader programming mode.\r\n");
#endif
			uQSFPState = QSFP_STATE_BOOTLOADER_PROGRAMMING_MODE;
		}
		else
		{
			// Start 5 second timer
			uQSFPStateCounter = 0x0;
			uQSFPState = QSFP_STATE_RESET;
		}
	}

	if ((Command->uProgram == 0x0)&&(uQSFPState == QSFP_STATE_BOOTLOADER_PROGRAMMING_MODE))
	{
		// If we were in programming mode and programming is now finished
		// Start 5 second timer and put in bootloader mode
		uQSFPStateCounter = 0x0;
		uQSFPState = QSFP_STATE_INITIAL_BOOTLOADER_MODE;

		uQSFPMezzanineCurrentTxLed = 0x0;
		uQSFPMezzanineCurrentRxLed = 0x0;
		uQSFPUpdateState = QSFP_UPDATING_TX_LEDS;
		uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

		uQSFPCtrlReg = QSFP0_RESET | QSFP1_RESET | QSFP2_RESET | QSFP3_RESET;
		WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);

#ifdef DEBUG_PRINT
		xil_printf("QSFP+ Mezzanine leaving bootloader programming mode.\r\n");
#endif
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
//	HMCReadI2CBytesCommandHandler
//--------------------------------------------------------------------------------
//	This method executes the HMC_READ_I2C command.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	pCommand				IN	Pointer to command header
//	uCommandLength			IN	Length of command
//	uResponsePacketPtr		IN	Pointer to where response packet must be constructed
//	uResponseLength			OUT	Length of payload of response packet
//
//	Return
//	------
//	XST_SUCCESS if successful
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
	//xil_printf("ID: %x SLV: %x ADDRS: %x %x %x %x\r\n", Command->uId, Command->uSlaveAddress, Command->uReadAddress[0], Command->uReadAddress[1], Command->uReadAddress[2], Command->uReadAddress[3]);
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
//	SDRAMProgramOverWishboneCommandHandler
//--------------------------------------------------------------------------------
//  This method executes the SDRAM_PROGRAM_OVER_WISHBONE command.
//
//	Return
//	------
//	XST_SUCCESS if successful
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

	sSDRAMProgramOverWishboneReqT *Command = (sSDRAMProgramOverWishboneReqT *) pCommand;
	sSDRAMProgramOverWishboneRespT *Response = (sSDRAMProgramOverWishboneRespT *) uResponsePacketPtr;

	if (uCommandLength < sizeof(sSDRAMProgramOverWishboneReqT)){
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

    // Check which Ethernet interfaces are part of IGMP groups
    // and send leave messages immediately
    for (uIndex = 0; uIndex < NUM_ETHERNET_INTERFACES; uIndex++)
    {
      if (uIGMPState[uIndex] == IGMP_STATE_JOINED_GROUP)
      {
        uIGMPState[uIndex] = IGMP_STATE_LEAVING;
        uIGMPSendMessage[uIndex] = IGMP_SEND_MESSAGE;
        uCurrentIGMPMessage[uIndex] = 0x0;
#ifdef DEBUG_PRINT
        xil_printf("IGMP[%02x]: About to send IGMP leave message.\r\n", uIndex);
#endif
      }
    }

    xil_printf("SDRAM PROGRAM[%02x] Chunk 0: about to clear sdram.\r\n", uId);
    uChunkIdCached = 0;
    ClearSdram();
		SetOutputMode(0x1, 0x1);
    /* Enable SDRAM Programming via Wishbone Bus */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_EN_REG_ADDRESS, 0x1);
    /* Set the Start SDRAM Programming control bit */
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_WB_PROGRAM_CTL_REG_ADDRESS, 0x2);

    uRetVal = XST_SUCCESS;

  } else if (Command->uChunkNum == (uChunkIdCached + 1)){
#ifdef DEBUG_PRINT
    /* xil_printf("chunk %d: about to write to sdram\r\n", Command->uChunkNum); */  /* this adds lots of overhead */
#endif
    for (uChunkByteIndex = 0; uChunkByteIndex < CHUNK_SIZE; uChunkByteIndex = uChunkByteIndex + 2){
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
      xil_printf("SDRAM PROGRAM[%02x] Chunk %d: about to end sdram write.\r\n", uId, Command->uChunkNum);

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
#ifdef DEBUG_PRINT
    xil_printf("SDRAM PROGRAM[%02x] Chunk %d: already received\r\n", uId, Command->uChunkNum);
#endif
  } else {
    uRetVal = XST_FAILURE;
  }

  return uRetVal;
}

int DHCPTuningDebugCommandHandler(struct sIFObject *pIFObj, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){
  struct sDHCPObject *pDHCPObj;
  u16 data[4] = {0};
  u16 rom[8];
  u8 uPaddingIndex;

  pDHCPObj = &(pIFObj->DHCPContextState);

  sDHCPTuningDebugReqT *Command = (sDHCPTuningDebugReqT *) pCommand;
	sDHCPTuningDebugRespT *Response = (sDHCPTuningDebugRespT *) uResponsePacketPtr;

	if (uCommandLength < sizeof(sDHCPTuningDebugReqT)){
		return XST_FAILURE;
  }

  Response->uStatus = 1;

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

  if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) != XST_SUCCESS){
    Response->uStatus = 0;
  } else {
    if (DS2433WriteMem(rom, 0, data, 4, 0xE0, 0x1, MB_ONE_WIRE_PORT) != XST_SUCCESS){
      Response->uStatus = 0;
    }
  }

	Response->Header.uCommandType = Command->Header.uCommandType + 1;
	Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

	Response->uInitTime = Command->uInitTime; 
	Response->uRetryTime = Command->uRetryTime; 

	for (uPaddingIndex = 0; uPaddingIndex < 6; uPaddingIndex++){
		Response->uPadding[uPaddingIndex] = 0;
  }

  *uResponseLength = sizeof(sDHCPTuningDebugRespT);

  pDHCPObj->uDHCPSMRetryInterval = Command->uRetryTime;
  pDHCPObj->uDHCPSMInitWait = Command->uInitTime;

#if 0
  vDHCPStateMachineReset(pIFObj);
  uDHCPSetStateMachineEnable(pIFObj, SM_TRUE);
#endif

  xil_printf("DHCP TUNING[%02x] Retry: %d, Init: %d\r\n", pIFObj->uIFEthernetId, Command->uRetryTime, Command->uInitTime);

  return XST_SUCCESS;
}
