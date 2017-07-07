/*
 * LLDP.h
 *
 *  Created on: 28 Jun 2017
 *      Author: sinethemba
 */

#ifndef SRC_LLDP_H_
#define SRC_LLDP_H_
#include<stdint.h>
/* some profitability stuff for datatypes used*/
#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

/*lldp packet offsets */
#define ETH_DST_OFFSET                           0
#define ETH_DST_LEN                              6
#define ETH_SRC_OFFSET                           (ETH_DST_OFFSET + ETH_DST_LEN) //6
#define ETH_SRC_LEN                              6
#define ETH_FRAME_TYPE_OFFSET                    (ETH_SRC_OFFSET + ETH_SRC_LEN) //12
#define ETH_FRAME_TYPE_LEN                       2

#define ETH_FRAME_TOTAL_LEN                      (ETH_FRAME_TYPE_OFFSET + ETH_FRAME_TYPE_LEN) /*ethernet length = 14*/

#define LLDP_CHASSIS_ID_TLV_TYPE_OFFSET          (ETH_FRAME_TYPE_OFFSET + ETH_FRAME_TYPE_LEN) //14
#define LLDP_CHASSIS_ID_TLV_TYPE_LEN             1
#define LLDP_CHASSIS_ID_TLV_LEN_OFFSET           (LLDP_CHASSIS_ID_TLV_TYPE_OFFSET + LLDP_CHASSIS_ID_TLV_TYPE_LEN) // 15
#define LLDP_CHASSIS_ID_TLV_LEN_LEN              1
#define LLDP_CHASSIS_ID_TLV_OFFSET               (LLDP_CHASSIS_ID_TLV_LEN_OFFSET + LLDP_CHASSIS_ID_TLV_LEN_LEN) // 16
#define LLDP_CHASSIS_ID_TLV_LEN                  7

#define LLDP_PORT_ID_TLV_TYPE_OFFSET             (LLDP_CHASSIS_ID_TLV_OFFSET + LLDP_CHASSIS_ID_TLV_LEN) //23
#define LLDP_PORT_ID_TLV_TYPE_LEN                1
#define LLDP_PORT_ID_TLV_LEN_OFFSET              (LLDP_PORT_ID_TLV_TYPE_OFFSET + LLDP_PORT_ID_TLV_TYPE_LEN)//24
#define LLDP_PORT_ID_TLV_LEN_LEN                 1
#define LLDP_PORT_ID_TLV_OFFSET                  (LLDP_PORT_ID_TLV_LEN_OFFSET + LLDP_PORT_ID_TLV_LEN_LEN) //25
#define LLDP_PORT_ID_TLV_LEN                     1

#define LLDP_TTL_TLV_TYPE_OFFSET                 (LLDP_PORT_ID_TLV_OFFSET + LLDP_PORT_ID_TLV_LEN) //26 + strlen(ip_address)
#define LLDP_TTL_TLV_TYPE_LEN                    1
#define LLDP_TTL_TLV_LEN_OFFSET                  (LLDP_TTL_TLV_TYPE_OFFSET + LLDP_TTL_TLV_TYPE_LEN) //27 + strlen(ip_address)
#define LLDP_TTL_TLV_LEN_LEN                     1
#define LLDP_TTL_TLV_OFFSET                      (LLDP_TTL_TLV_LEN_OFFSET + LLDP_TTL_TLV_LEN_LEN) //28 + strlen(ip_address)
#define LLDP_TTL_TLV_LEN                         2

#define LLDP_PORT_DESCR_TLV_TYPE_OFFSET          (LLDP_TTL_TLV_OFFSET + LLDP_TTL_TLV_LEN) //30 + strlen(ip_address)
#define LLDP_PORT_DESCR_TLV_TYPE_LEN             1
#define LLDP_PORT_DESCR_TLV_LEN_OFFSET           (LLDP_PORT_DESCR_TLV_TYPE_OFFSET + LLDP_PORT_DESCR_TLV_TYPE_LEN) //31 + strlen(ip_address)
#define LLDP_PORT_DESCR_TLV_LEN_LEN              1
#define LLDP_PORT_DESCR_TLV_OFFSET               (LLDP_PORT_DESCR_TLV_LEN_OFFSET + LLDP_PORT_DESCR_TLV_LEN_LEN) //32 + strlen(ip_address)
#define LLDP_PORT_DESCR_TLV_LEN                  12

#define LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET       (LLDP_PORT_DESCR_TLV_OFFSET + LLDP_PORT_DESCR_TLV_LEN) // 44 + strlen(ip_address)
#define LLDP_END_OF_LLDPDU_TLV_TYPE_LEN          1
#define LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET        (LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET + LLDP_END_OF_LLDPDU_TLV_TYPE_LEN) //45 + strlen(ip_address)
#define LLDP_END_OF_LLDPDU_TLV_LEN_LEN           1
#define LLDP_END_OF_LLDPDU_TLV_OFFSET            (LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET + LLDP_END_OF_LLDPDU_TLV_LEN_LEN) //46 + + strlen(ip_address)

enum LLDP_TLV_TYPE {

	/* start to mandatory TLV */
	LLDP_END_OF_LLDPDU_TLV = 0,
	LLDP_CHASSIS_ID_TLV = 2,
	LLPD_PORT_ID_TLV = 4,
	LLDP_TTL_TLV = 6,
	/* end of mandatory TLV */

	/* start of optional TLV */
	LLDP_PORT_DESC_TLV = 8,

};

enum LLDP_CHASSIS_ID_SUBTYPE {
	LLDP_CHASSIS_ID_MAC_ADDRESS = 4
};

enum LLDP_PORT_ID_SUBTYPE {
	LLDP_PORT_ID_NETWORK_ADDRESS = 4
};

void uLLDPBuildPacket(u8 uId, u8 *pTransmitBuffer, u32 *uResponseLength);
void  uMY_IP_Address(u32 ip, u8 ip_addr[], char IP_Buffer[]);
void uIP_TO_String(char IP_Buffer[], u8 ip_addr[]);
void uToStringHex(char str[], uint32_t num);
int _uPower(int x, unsigned int y);



#endif /* SRC_LLDP_H_ */

