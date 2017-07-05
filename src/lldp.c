/*
 * LLDP.c
 *
 *  Created on: 28 Jun 2017
 *      Author: sinethemba
 */

/* standard C lib includes */
#include <stdio.h>
#include <math.h>
#include <string.h>


/* local includes */
#include "LLDP.h"
#include "constant_defs.h"


/* Local function prototypes*/

/***********************LLDP API functions***************************/
//=================================================================================
//  uLLDPBuildPacket
//---------------------------------------------------------------------------------
// This method builds the LLDP Packet into the user supplied buffer
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uResponseLength		OUT	Length of packet created
//
//	Return
//	------
//	None
//==================================================================================
void uLLDPBuildPacket(u8 uId, u8 *pTransmitBuffer, u32 *uResponseLength){

	u8 dst_mac_addr[ETH_DST_LEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };

	u8 src_mac_addr[ETH_SRC_LEN];
	src_mac_addr[0] = ((uEthernetFabricMacHigh[uId] >> 8) & 0xFF);
	src_mac_addr[1] = (uEthernetFabricMacHigh[uId] & 0xFF);
	src_mac_addr[2] = ((uEthernetFabricMacMid[uId] >> 8) & 0xFF);
	src_mac_addr[3] = (uEthernetFabricMacMid[uId] & 0xFF);
	src_mac_addr[4] = ((uEthernetFabricMacLow[uId] >> 8) & 0xFF);
	src_mac_addr[5] = (uEthernetFabricMacLow[uId] & 0xFF);

	/* zero the buffer, saves us from having to explicitly set zero valued bytes */
	memset(pTransmitBuffer, 0, 1024);

	/******* Ethernet frame stuff ******/

	/* fill in our hardware multicast mac address(destination address) */
	memcpy(pTransmitBuffer + ETH_DST_OFFSET, dst_mac_addr, ETH_DST_LEN);

	/* fill in our src hardware mac address */
	memcpy(pTransmitBuffer + ETH_SRC_OFFSET, src_mac_addr, ETH_SRC_LEN);

	/* ethernet frame type */
	pTransmitBuffer[ETH_FRAME_TYPE_OFFSET] = ((ETHERNET_TYPE_LLDP >> 8) & 0xFF);
	pTransmitBuffer[ETH_FRAME_TYPE_OFFSET + 1] = (ETHERNET_TYPE_LLDP & 0xFF);

	/* chassis ID TLV */
	pTransmitBuffer[LLDP_CHASSIS_ID_TLV_TYPE_OFFSET] = LLDP_CHASSIS_ID_TLV;
	pTransmitBuffer[LLDP_CHASSIS_ID_TLV_LEN_OFFSET] = LLDP_CHASSIS_ID_TLV_LEN;
	pTransmitBuffer[LLDP_CHASSIS_ID_TLV_OFFSET] = LLDP_CHASSIS_ID_MAC_ADDRESS;
	memcpy(pTransmitBuffer + LLDP_CHASSIS_ID_TLV_OFFSET + 1, src_mac_addr, 6);

	/* port ID TLV*/
	u32 uIP = uEthernetFabricIPAddress[uId];
	u8 IP_Addr[4] = {0};
	char IP_Buffer[16];
	uMY_IP_Address(uIP, IP_Addr, IP_Buffer);
	pTransmitBuffer[LLDP_PORT_ID_TLV_TYPE_OFFSET] =  LLPD_PORT_ID_TLV;
	pTransmitBuffer[LLDP_PORT_ID_TLV_LEN_OFFSET] =  LLDP_PORT_ID_TLV_LEN + strlen(IP_Buffer);
	pTransmitBuffer[LLDP_PORT_ID_TLV_OFFSET] = LLDP_PORT_ID_NETWORK_ADDRESS;
	memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, IP_Buffer, strlen(IP_Buffer));

	/* Tome to live TLV */
	pTransmitBuffer[LLDP_TTL_TLV_TYPE_OFFSET + strlen(IP_Buffer)] = LLDP_TTL_TLV;
	pTransmitBuffer[LLDP_TTL_TLV_LEN_OFFSET + strlen(IP_Buffer)] = LLDP_TTL_TLV_LEN;
	pTransmitBuffer[LLDP_TTL_TLV_OFFSET + strlen(IP_Buffer)] = 0;
	pTransmitBuffer[LLDP_TTL_TLV_OFFSET + 1 + strlen(IP_Buffer)] = 120;

	/* port description TLV */
	pTransmitBuffer[LLDP_PORT_DESCR_TLV_TYPE_OFFSET + strlen(IP_Buffer)] = LLDP_PORT_DESC_TLV;
	pTransmitBuffer[LLDP_PORT_DESCR_TLV_LEN_OFFSET + strlen(IP_Buffer)] = LLDP_PORT_DESCR_TLV_LEN;
	switch(uId){
	case 0:
		memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET + strlen(IP_Buffer), ONE_GBE_INTERFACE, LLDP_PORT_DESCR_TLV_LEN);
		break;

	case 1:
		memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET + strlen(IP_Buffer), FORTY_GBE_INTERFACE, LLDP_PORT_DESCR_TLV_LEN);
		break;
	}

	/* LLDP END OF LLDPDU TLV */
	pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET + strlen(IP_Buffer)] = LLDP_END_OF_LLDPDU_TLV;
	pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET + strlen(IP_Buffer)] = 0;

	*uResponseLength = sizeof(pTransmitBuffer) * 16;


}

//=================================================================================
//  uLLDPBuildPacket
//---------------------------------------------------------------------------------
// This method builds the LLDP Packet into the user supplied buffer
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	uId					IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uResponseLength		OUT	Length of packet created
//
//	Return
//	------
//	None
//==================================================================================

void  uMY_IP_Address(u32 uIP, u8 IP_Addr[], char IP_Buffer[]){
	char hex[17];
	u32 decimal;
	int i = 0, val, len;

	decimal = 0;
	uToStringHex(hex, uIP);

	/* Find the length of total number of hex digit */
	len = strlen(hex);
	len--;

	/* Iterate over each hex digit */
	for(i = 0; hex[i] != '\0'; i++)
	{
		/* Find the decimal representation of hex[i]*/
		if(hex[i] >= '0' && hex[i] <= '9')
		{
			val = hex[i] - 48;
		}
		else if(hex[i] >= 'a' && hex[i] <= 'f')
		{
			val = hex[i] - 97 + 10;
		}
		else if(hex[i] >= 'A' && hex[i] <= 'F')
		{
			val = hex[i] - 65 + 10;
		}
		decimal += val * uPower(16, len);
		len--;
	}

	//u8 ip_addr[4];
	IP_Addr[3] = (decimal & 0x000000ff);
	IP_Addr[2] = (decimal & 0x0000ff00) >> 8;
	IP_Addr[1] = (decimal & 0x00ff0000) >> 16;
	IP_Addr[0] = (decimal & 0xff000000) >> 24;

	uIP_TO_String(IP_Buffer, IP_Addr);


}

void uToStringHex(char str[], uint32_t uNum)

{
	unsigned char lookup[16] = "0123456789abcdef";
	unsigned int i, b;
	int len = 0;

	for (i = 0; i < 32; i+=4)
	{
		b = (uNum >> (28 - i) & 0xf);
		str[len] = lookup[b];
		len++;
	}
	str[len] = '\0';

}

void uIP_TO_String(char IP_Buffer[], u8 IP_Addr[]){
	unsigned char lookup[10] = "0123456789";
	//uint8_t a[4] = {192, 168, 14, 5};
	unsigned int i, b, c, k;
	int leading;
	//char ip[16];
	int len = 0;
	for(k = 0; k < sizeof(IP_Addr); k++){
		leading = 0;
		b = IP_Addr[k];

		for(i = 1000 ; i > 9 ; i = i / 10){
			c = b / i;
			if(c || leading){
				IP_Buffer[len] = lookup[c];
				leading = 1;
				len++;
			}
			b = b - (i * c);
		}
		IP_Buffer[len++] = lookup[b];
		if(k != 3)
			IP_Buffer[len++] = '.';
		else
			IP_Buffer[len++] = '\0';

	}
}

int uPower(int base, unsigned int exp)
{
	if( exp == 0)
		return 1;

	int t = uPower(base, exp/2);  // power is called only once instead of twice.

	if (exp%2 == 0)
		return t*t;
	else
		return base*t*t;
}

