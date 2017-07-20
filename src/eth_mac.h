
#ifndef ETH_MAC_H_
#define ETH_MAC_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : eth_mac.h
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
*  This file contains the definitions and constants for accessing the ETH MAC
*  core over the Wishbone bus (used for both 1GBE and 40GBE MAC).
* ------------------------------------------------------------------------------*/

// INCLUDES

#include <stdio.h>
#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"

#define ETH_MAC_SOFT_RESET			0x01000000
#define ETH_MAC_ENABLE				0x00010000

#define ETH_MAC_RESET_TIMEOUT			1000

#define ETH_MAC_REG_SOURCE_MAC_UPPER_16			0x0u
#define ETH_MAC_REG_SOURCE_MAC_LOWER_32			0x1u
#define ETH_MAC_REG_GATEWAY						0x3u
#define ETH_MAC_REG_SOURCE_IP_ADDRESS			0x4u
#define ETH_MAC_REG_NETMASK         0xEu
#define ETH_MAC_REG_BUFFER_LEVEL				0x6u
#define ETH_MAC_REG_SOURCE_PORT_AND_ENABLE		0x8u
#define ETH_MAC_REG_XAUI_STATUS					0x9u
#define ETH_MAC_REG_PHY_CONFIGURATION			0xAu
#define ETH_XAUI_CONFIG 						0xBu
#define ETH_MC_RECV_IP 							0xCu
#define ETH_MC_RECV_IP_MASK						0xDu

#define ETH_MAC_REG_LOW_ADDRESS						0x0000u
#define ETH_MAC_REG_HIGH_ADDRESS					0x07FFu
#define ETH_MAC_CPU_TRANSMIT_BUFFER_LOW_ADDRESS		0x1000u
#define ETH_MAC_CPU_TRANSMIT_BUFFER_HIGH_ADDRESS	0x17FFu
#define ETH_MAC_CPU_RECEIVE_BUFFER_LOW_ADDRESS		0x2000u
#define ETH_MAC_CPU_RECEIVE_BUFFER_HIGH_ADDRESS		0x27FFu
#define ETH_MAC_ARP_CACHE_LOW_ADDRESS				0x3000u
#define ETH_MAC_ARP_CACHE_HIGH_ADDRESS				0x37FFu

#define ETH_MAC_HOST_PACKET_TRANSMIT_TIMEOUT		1000

u32 GetAddressOffset(u8 uId);
int SoftReset(u8 uId);
void SetFabricSourceMACAddress(u8 uId, u16 uMACAddressUpper16Bits, u32 uMACAddressLower32Bits);
void SetFabricGatewayARPCacheAddress(u8 uId, u8 uGatewayARPCacheAddress);
void SetFabricSourceIPAddress(u8 uId, u32 uIPAddress);
void SetFabricNetmask(u8 uId, u32 uNetmask);
void SetFabricSourcePortAddress(u8 uId, u16 uPortAddress);
void SetMultiCastIPAddress(u8 uId, u32 uMultiCastIPAddress, u32 uMultiCastIPAddressMask);
void EnableFabricInterface(u8 uId, u8 uEnable);
void ProgramARPCacheEntry(u8 uId, u32 uIPAddressLower8Bits, u32 uMACAddressUpper16Bits, u32 uMACAddressLower32Bits);
u32 GetHostTransmitBufferLevel(u8 uId);
u32 GetHostReceiveBufferLevel(u8 uId);
void SetHostTransmitBufferLevel(u8 uId, u16 uBufferLevel);
void AckHostPacketReceive(u8 uId);
int TransmitHostPacket(u8 uId, volatile u32 *puTransmitPacket, u32 uNumWords);	// GT 31/03/2017 INSTRUCT COMPILER BUFFER IS VOLATILE
int ReadHostPacket(u8 uId, volatile u32 *puReceivePacket, u32 uNumWords); // GT 31/03/2017 INSTRUCT COMPILER BUFFER IS VOLATILE

#endif
