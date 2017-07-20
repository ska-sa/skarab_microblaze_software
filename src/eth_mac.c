/**------------------------------------------------------------------------------
*  FILE NAME            : eth_mac.c
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
*  This file contains the implementation of functions to access the ETH MAC core
*  over the Wishbone bus.
* ------------------------------------------------------------------------------*/

#include "eth_mac.h"

//=================================================================================
//	GetAddressOffset
//--------------------------------------------------------------------------------
//	This method gets the offset address of the selected ETH MAC on the Wishbone bus.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want offset address for
//
//	Return
//	------
//	Wishbone bus address offset
//=================================================================================
u32 GetAddressOffset(u8 uId)
{
	if (uId == 0)
		return ONE_GBE_MAC_ADDR;
	else if (uId == 1)
		return FORTY_GBE_MAC_0_ADDR;
	else if (uId == 2)
		return FORTY_GBE_MAC_1_ADDR;
	else if (uId == 3)
		return FORTY_GBE_MAC_2_ADDR;
	else
		return FORTY_GBE_MAC_3_ADDR;

}

//=================================================================================
//	SoftReset
//--------------------------------------------------------------------------------
//	This method performs a soft reset of the ETH MAC core.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to reset
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int SoftReset(u8 uId)
{
	unsigned uTimeout = 0x0;
	u32 uStatus = 0x0;

	u32 uAddressOffset = GetAddressOffset(uId);

	// Assert soft reset bit
	Xil_Out8(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_PORT_AND_ENABLE) + 3, 0x1);

	// Wait for reset bit to be cleared indicating that the reset is complete
	do
	{
		uStatus = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + ONE_GBE_MAC_ADDR + (4*ETH_MAC_REG_SOURCE_PORT_AND_ENABLE));
		uTimeout++;
	}
	while(((uStatus & ETH_MAC_SOFT_RESET) != 0x0)&&(uTimeout < ETH_MAC_RESET_TIMEOUT));

	if (uTimeout == ETH_MAC_RESET_TIMEOUT)
		return XST_FAILURE;

	return XST_SUCCESS;

}

//=================================================================================
//	SetFabricSourceMACAddress
//--------------------------------------------------------------------------------
//	This method sets the fabric source MAC address of the ETH MAC core.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId						IN	ID of ETH MAC want to set fabric MAC address
//	uMACAddressUpper16Bits	IN	Upper 16 bits of the source MAC address
//	uMACAddressLower32Bits 	IN	Lower 32 bits of the source MAC address
//
//	Return
//	------
//	None
//=================================================================================
void SetFabricSourceMACAddress(u8 uId, u16 uMACAddressUpper16Bits, u32 uMACAddressLower32Bits)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_MAC_UPPER_16), uMACAddressUpper16Bits);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_MAC_LOWER_32), uMACAddressLower32Bits);

}

//=================================================================================
//	SetFabricGatewayARPCacheAddress
//--------------------------------------------------------------------------------
//	This method sets the location in the ARP cache of the gateway MAC address.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId						IN 	ID of ETH MAC want to set gateway MAC address location
//	uGatewayARPCacheAddress	IN	Location in ARP cache
//
//	Return
//	------
//	None
//=================================================================================
void SetFabricGatewayARPCacheAddress(u8 uId, u8 uGatewayARPCacheAddress)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out8(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_GATEWAY), uGatewayARPCacheAddress);

}

//=================================================================================
//	SetFabricSourceIPAddress
//--------------------------------------------------------------------------------
//	This method sets the source IP address of the FPGA fabric.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to set source fabric IP address
//	uIPAddress	IN		Source IP address of the FPGA fabric
//
//	Return
//	------
//	None
//=================================================================================
void SetFabricSourceIPAddress(u8 uId, u32 uIPAddress)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_IP_ADDRESS), uIPAddress);

}

//=================================================================================
//  SetFabricNetmask
//--------------------------------------------------------------------------------
//  This method sets the netmask of the FPGA fabric.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN    ID of the ETH MAC to set netmask of
//  uNetmask  IN    Netmask of FPGA fabric
//
//  Return
//  ------
//  None
//=================================================================================
void SetFabricNetmask(u8 uId, u32 uNetmask)
{
  u32 uAddressOffset = GetAddressOffset(uId);

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_NETMASK), uNetmask);
}

//=================================================================================
//	SetMultiCastIPAddress
//--------------------------------------------------------------------------------
//	This method sets the Multicast IP address parameters.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId						IN	ID of ETH MAC want to set Multicast IP parameters for
//	uMultiCastIPAddress		IN	Multicast IP address
//	uMultiCastIPAddressMask	IN	Mask to select portion of multicast IP address that applies
//
//	Return
//	------
//	None
//=================================================================================
void SetMultiCastIPAddress(u8 uId, u32 uMultiCastIPAddress, u32 uMultiCastIPAddressMask)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MC_RECV_IP), uMultiCastIPAddress);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MC_RECV_IP_MASK), uMultiCastIPAddressMask);

}

//=================================================================================
//	SetFabricSourcePortAddress
//--------------------------------------------------------------------------------
//	This method sets the source port address of the FPGA fabric.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to set fabric source port address for
//	uIPAddress	IN		Source IP address of the FPGA fabric
//
//	Return
//	------
//	None
//=================================================================================
void SetFabricSourcePortAddress(u8 uId, u16 uPortAddress)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_PORT_AND_ENABLE), uPortAddress);

}

//=================================================================================
//	EnableFabricInterface
//--------------------------------------------------------------------------------
//	This method is used to enable or disable the FPGA fabric interface.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to enable/disable
//	uEnable		IN		1 = enable, 0 = disable
//
//	Return
//	------
//	None
//=================================================================================
void EnableFabricInterface(u8 uId, u8 uEnable)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out8(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_SOURCE_PORT_AND_ENABLE) + 2, uEnable);

}

//=================================================================================
//	ProgramARPCacheEntry
//--------------------------------------------------------------------------------
//	This method is used to program an entry in the ARP cache.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId						IN 	ID of ETH MAC want to program ARP cache for
//	uIPAddressLower8Bits	IN	Lower 8 bits of destination IP address to index into table
//	uMACAddressUpper16Bits	IN	Upper 16 bits of destination MAC address for corresponding IP address
//	uMACAddressLower32Bits	IN	Lower 32 bits of destination MAC address for corresponding IP address
//
//	Return
//	------
//	None
//=================================================================================
void ProgramARPCacheEntry(u8 uId, u32 uIPAddressLower8Bits, u32 uMACAddressUpper16Bits, u32 uMACAddressLower32Bits)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_ARP_CACHE_LOW_ADDRESS + (4*(2 * uIPAddressLower8Bits)), uMACAddressUpper16Bits);
	Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_ARP_CACHE_LOW_ADDRESS + (4*((2 * uIPAddressLower8Bits) + 1)), uMACAddressLower32Bits);

}

//=================================================================================
//	GetHostTransmitBufferLevel
//--------------------------------------------------------------------------------
//	This method is used to get the current host transmit FIFO level in 32 bit words.
//	Value read is in 64 bit words so multiplied by 2.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to get transmit buffer level of
//
//	Return
//	------
//	Transmit buffer level
//=================================================================================
u32 GetHostTransmitBufferLevel(u8 uId)
{
	u32 uReg = 0x0;
	u32 uAddressOffset = GetAddressOffset(uId);

	uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_BUFFER_LEVEL));

	uReg = (uReg >> 16) & 0xFF;

	return (2 * uReg);

}

//=================================================================================
//	GetHostReceiveBufferLevel
//--------------------------------------------------------------------------------
//	This method is used to get the current host receive FIFO level in 32 bit words.
//	Value read is in 64 bit words so multiplied by 2.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC want to get receive buffer level of
//
//	Return
//	------
//	Receive buffer level
//=================================================================================
u32 GetHostReceiveBufferLevel(u8 uId)
{
	u32 uReg = 0x0;
	u32 uAddressOffset = GetAddressOffset(uId);

	uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_BUFFER_LEVEL));

	return (2 * (uReg & 0xFF));

}

//=================================================================================
//	SetHostTransmitBufferLevel
//--------------------------------------------------------------------------------
//	This method is used to set the current host transmit FIFO level in 32 bit words.
//	Value written is in 64 bit words so divided by 2.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId				IN	ID of ETH MAC want to set current transmit buffer level to
//	uBufferLevel	IN
//
//	Return
//	------
//	None
//=================================================================================
void SetHostTransmitBufferLevel(u8 uId, u16 uBufferLevel)
{
	u16 uReg = uBufferLevel / 2;
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_BUFFER_LEVEL) + 2, uReg);

}

//=================================================================================
//	AckHostPacketReceive
//--------------------------------------------------------------------------------
//	This method is used to acknowledge that the received host packet has been read.
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId			IN		ID of ETH MAC that want to ACK host packet received
//
//	Return
//	------
//	None
//=================================================================================
void AckHostPacketReceive(u8 uId)
{
	u32 uAddressOffset = GetAddressOffset(uId);

	Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (4*ETH_MAC_REG_BUFFER_LEVEL), 0x0);

}

//=================================================================================
//	TransmitHostPacket
//--------------------------------------------------------------------------------
//	This method is used to send a packet from the host (MicroBlaze).
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	ID of ETH MAC that want to send packet from
//	puTransmitPacket	IN	Pointer to contents of packet to transmit
//	uNumWords			IN	Number of 32-bit words in transmit packet
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int TransmitHostPacket(u8 uId, volatile u32 *puTransmitPacket, u32 uNumWords)
{
	unsigned uTimeout = 0x0;
	u32 uHostTransmitBufferLevel = 0x0;
	u32 uIndex = 0x0;
	u32 uAddressOffset = GetAddressOffset(uId);
	//u32 uReg;

	// Must be a multiple of 64 bits
	if ((uNumWords % 2) != 0x0)
	{
		xil_printf("TransmitHostPacket: Packet size must be multiple of 64 bits SIZE: %x.\r\n", uNumWords);
		return XST_FAILURE;
	}

	// Check that the transmit buffer is ready for a packet
	do
	{
		uHostTransmitBufferLevel = GetHostTransmitBufferLevel(uId);
		uTimeout++;
	}
	while((uHostTransmitBufferLevel != 0x0)&&(uTimeout < ETH_MAC_HOST_PACKET_TRANSMIT_TIMEOUT));

	if (uTimeout == ETH_MAC_HOST_PACKET_TRANSMIT_TIMEOUT)
  {
		xil_printf("TransmitHostPacket: Timeout waiting for transmit buffer to be empty. LEVEL: %x\r\n", uHostTransmitBufferLevel);
		return XST_FAILURE;
  }

	// Program transmit packet words into FIFO
	for (uIndex = 0x0; uIndex < uNumWords; uIndex++)
	{
		//uReg = ((puTransmitPacket[uIndex] >> 16) & 0xFFFF) | ((puTransmitPacket[uIndex] & 0xFFFF) << 16);
		//Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_CPU_TRANSMIT_BUFFER_LOW_ADDRESS + 4*uIndex, uReg);
		Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_CPU_TRANSMIT_BUFFER_LOW_ADDRESS + 4*uIndex, puTransmitPacket[uIndex]);
	}

	// Program the packet size into buffer level register to trigger start of packet transmission
	SetHostTransmitBufferLevel(uId, uNumWords);

	// Wait for the packet to be transmitted
	do
	{
		uHostTransmitBufferLevel = GetHostTransmitBufferLevel(uId);
		uTimeout++;
	}
	while((uHostTransmitBufferLevel != 0x0)&&(uTimeout < ETH_MAC_HOST_PACKET_TRANSMIT_TIMEOUT));

	if (uTimeout == ETH_MAC_HOST_PACKET_TRANSMIT_TIMEOUT)
  {
		xil_printf("TransmitHostPacket: Timeout waiting for packet to be sent. LEVEL: %x\r\n", uHostTransmitBufferLevel);
		return XST_FAILURE;
  }

	return XST_SUCCESS;

}

//=================================================================================
//	ReadHostPacket
//--------------------------------------------------------------------------------
//	This method is used to read a packet from the host (MicroBlaze).
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId				IN	ID of ETH MAC that want to read a packet from
//	puReceivePacket	OUT	Pointer to location where to store received packet
//	uNumWords		IN	Number of 32-bit words to read
//
//	Return
//	------
//	XST_SUCCESS if successful
//=================================================================================
int ReadHostPacket(u8 uId, volatile u32 *puReceivePacket, u32 uNumWords)
{
	unsigned uIndex = 0x0;
	u32 uAddressOffset = GetAddressOffset(uId);
	//u32 uReg;

	// GT 31/03/2017 NEED TO CHECK THAT DON'T OVERFLOW ReceivePacket ARRAY
	//if (uNumWords > GetHostReceiveBufferLevel(uId))
	//{
	//	xil_printf("ReadHostPacket: Packet too big!\r\n");
	//	return XST_FAILURE;
	//}

	if (uNumWords > 512)
	{
		xil_printf("ReadHostPacket: Packet size exceeds 512 words. SIZE: %x\r\n", uNumWords);
		return XST_FAILURE;
	}

	for (uIndex = 0x0; uIndex < uNumWords; uIndex++)
	{
		puReceivePacket[uIndex] = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_CPU_RECEIVE_BUFFER_LOW_ADDRESS + (4*uIndex));
		//uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + ETH_MAC_CPU_RECEIVE_BUFFER_LOW_ADDRESS + (4*uIndex));
		//puReceivePacket[uIndex] = ((uReg & 0xFFFF) << 16) | ((uReg >> 16) & 0xFFFF);
	}

	// Acknowledge reading the packet from the FIFO
	AckHostPacketReceive(uId);

	return XST_SUCCESS;
}
