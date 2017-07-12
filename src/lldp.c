/*
 * LLDP.c
 *
 *  Created on: 28 Jun 2017
 *      Author: sinethemba
 */

/* standard C lib includes */
#include <string.h>

/* local includes */
#include "lldp.h"
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
//	uId			IN	Selected Ethernet interface
//	pTransmitBuffer		OUT	Location to create packet
//	uResponseLength		OUT	Length of packet created
//
//	Return
//	------
//	None
//==================================================================================
int uLLDPBuildPacket(u8 uId, u8 *pTransmitBuffer, u32 *uResponseLength){
	
	u32 uIPAddress = uEthernetFabricIPAddress[uId];
	u16 uIPLength;
	u32 version;
	u8 dst_mac_addr[ETH_DST_LEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e };

	u8 src_mac_addr[ETH_SRC_LEN];
	src_mac_addr[0] = ((uEthernetFabricMacHigh[uId] >> 8) & 0xFF);
	src_mac_addr[1] = (uEthernetFabricMacHigh[uId] & 0xFF);
	src_mac_addr[2] = ((uEthernetFabricMacMid[uId] >> 8) & 0xFF);
	src_mac_addr[3] = (uEthernetFabricMacMid[uId] & 0xFF);
	src_mac_addr[4] = ((uEthernetFabricMacLow[uId] >> 8) & 0xFF);
	src_mac_addr[5] = (uEthernetFabricMacLow[uId] & 0xFF);

	/* zero the buffer, saves us from having to explicitly set zero valued bytes */
	memset(pTransmitBuffer, 0, LLDP_MAX_BUFFER_SIZE);

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
	uIPAddr[3] = (uIPAddress & 0x000000ff);
        uIPAddr[2] = (uIPAddress & 0x0000ff00) >> 8;
        uIPAddr[1] = (uIPAddress & 0x00ff0000) >> 16;
        uIPAddr[0] = (uIPAddress & 0xff000000) >> 24;
        uIPLength = uIPToString(pIPBuffer, uIPAddr);
	if(uIPLength > 0){
		pTransmitBuffer[LLDP_PORT_ID_TLV_TYPE_OFFSET] =  LLPD_PORT_ID_TLV;
		pTransmitBuffer[LLDP_PORT_ID_TLV_LEN_OFFSET] =  LLDP_PORT_ID_TLV_LEN + uIPLength;
		pTransmitBuffer[LLDP_PORT_ID_TLV_OFFSET] = LLDP_PORT_ID_NETWORK_ADDRESS;
		memcpy(pTransmitBuffer + LLDP_PORT_ID_TLV_OFFSET + 1, pIPBuffer, uIPLength);
	}else{
		return LLDP_RETURN_INVALID;
	}
	/* Tome to live TLV */
	pTransmitBuffer[LLDP_TTL_TLV_TYPE_OFFSET + uIPLength] = LLDP_TTL_TLV;
	pTransmitBuffer[LLDP_TTL_TLV_LEN_OFFSET + uIPLength] = LLDP_TTL_TLV_LEN;
	pTransmitBuffer[LLDP_TTL_TLV_OFFSET + uIPLength] = 0;
	pTransmitBuffer[LLDP_TTL_TLV_OFFSET + 1 + uIPLength] = 120;

	/* port description TLV */
	pTransmitBuffer[LLDP_PORT_DESCR_TLV_TYPE_OFFSET + uIPLength] = LLDP_PORT_DESC_TLV;
	pTransmitBuffer[LLDP_PORT_DESCR_TLV_LEN_OFFSET + uIPLength] = LLDP_PORT_DESCR_TLV_LEN;
	switch(uId){
	case 0:
		memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET + uIPLength, ONE_GBE_INTERFACE, LLDP_PORT_DESCR_TLV_LEN);
		break;

	case 1:
		memcpy(pTransmitBuffer + LLDP_PORT_DESCR_TLV_OFFSET + uIPLength, FORTY_GBE_INTERFACE, LLDP_PORT_DESCR_TLV_LEN);
		break;
	}

	/* LLDP_SYSTEM_DESCR_TLV */
	pTransmitBuffer[LLDP_SYSTEM_DESCR_TLV_TYPE_OFFSET + uIPLength] = LLDP_SYSTEM_DESCR_TLV;
	pTransmitBuffer[LLDP_SYSTEM_DESCR_TLV_LEN_OFFSET + uIPLength] = LLDP_SYSTEM_DESCR_TLV_LEN;
	version = ReadBoardRegister(C_RD_VERSION_ADDR);
	u16 first_part = (version & 0xff000000) >> 24; // for BSP or TLF
	if(first_part > 0){
		memcpy(pTransmitBuffer + LLDP_SYSTEM_DESCR_TLV_OFFSET + uIPLength, "BSP", LLDP_SYSTEM_DESCR_TLV_LEN);
	}
	else
		 memcpy(pTransmitBuffer + LLDP_SYSTEM_DESCR_TLV_OFFSET + uIPLength, "TLF", LLDP_SYSTEM_DESCR_TLV_LEN);
	/* LLDP END OF LLDPDU TLV */
	pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET + uIPLength] = LLDP_END_OF_LLDPDU_TLV;
	pTransmitBuffer[LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET + uIPLength] = 0;

	*uResponseLength = LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET + uIPLength + 1;
	
	return LLDP_RETURN_OK;

}

//=================================================================================
//  uIPToString
//---------------------------------------------------------------------------------
//  This method converts a u32 hex IP address to a string in dot notation
//
//      Parameter       Dir             Description
//      ---------       ---             -----------
//      pIPBuffer       OUT      Location to store string IP address as a string in dot notation
//      pIPAddr         IN       u8 pieces of IP address
//
//      Return
//      ------
//      len                        Length of IP address in dot notation
//==================================================================================

int uIPToString(char* pIPBuffer, u8* pIPAddr){
	unsigned char lookup[10] = "0123456789";
	unsigned int i, b, c, k;
	int leading;
	int len = 0;

	if(pIPAddr == NULL){
		return LLDP_RETURN_FAIL;
	}
	for(k = 0; k < 4; k++){
		leading = 0;
		b = pIPAddr[k];

		for(i = 1000 ; i > 9 ; i = i / 10){
			c = b / i;
			if(c || leading){
				pIPBuffer[len] = lookup[c];
				leading = 1;
				len++;
			}
			b = b - (i * c);
		}
		pIPBuffer[len++] = lookup[b];
		if(k != 3)
			pIPBuffer[len++] = '.';
		else
			pIPBuffer[len++] = '\0';

	}
	len--;
	return len;
}
