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

/*link custom return values */
#define LLDP_RETURN_FAIL     XST_FAILURE
#define LLDP_RETURN_OK       XST_SUCCESS
#define LLDP_RETURN_INVALID  XST_FAILURE
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

#define LLDP_TTL_TLV_TYPE_OFFSET                 (LLDP_PORT_ID_TLV_OFFSET + LLDP_PORT_ID_TLV_LEN) //26 + uIPLength
#define LLDP_TTL_TLV_TYPE_LEN                    1
#define LLDP_TTL_TLV_LEN_OFFSET                  (LLDP_TTL_TLV_TYPE_OFFSET + LLDP_TTL_TLV_TYPE_LEN) //27 + uIPLength
#define LLDP_TTL_TLV_LEN_LEN                     1
#define LLDP_TTL_TLV_OFFSET                      (LLDP_TTL_TLV_LEN_OFFSET + LLDP_TTL_TLV_LEN_LEN) //28 + uIPLength
#define LLDP_TTL_TLV_LEN                         2

#define LLDP_PORT_DESCR_TLV_TYPE_OFFSET          (LLDP_TTL_TLV_OFFSET + LLDP_TTL_TLV_LEN) //30 + uIPLength
#define LLDP_PORT_DESCR_TLV_TYPE_LEN             1
#define LLDP_PORT_DESCR_TLV_LEN_OFFSET           (LLDP_PORT_DESCR_TLV_TYPE_OFFSET + LLDP_PORT_DESCR_TLV_TYPE_LEN) //31 + uIPLength
#define LLDP_PORT_DESCR_TLV_LEN_LEN              1
#define LLDP_PORT_DESCR_TLV_OFFSET               (LLDP_PORT_DESCR_TLV_LEN_OFFSET + LLDP_PORT_DESCR_TLV_LEN_LEN) //32 + uIPLength
#define LLDP_PORT_DESCR_TLV_LEN                  12

#define LLDP_SYSTEM_DESCR_TLV_TYPE_OFFSET        (LLDP_PORT_DESCR_TLV_OFFSET + LLDP_PORT_DESCR_TLV_LEN) //44 + uIPLength
#define LLDP_SYSTEM_DESCR_TLV_TYPE_LEN           1
#define LLDP_SYSTEM_DESCR_TLV_LEN_OFFSET         (LLDP_SYSTEM_DESCR_TLV_TYPE_OFFSET + LLDP_SYSTEM_DESCR_TLV_TYPE_LEN) //45 + uIPLength
#define LLDP_SYSTEM_DESCR_TLV_LEN_LEN            1
#define LLDP_SYSTEM_DESCR_TLV_OFFSET             (LLDP_SYSTEM_DESCR_TLV_LEN_OFFSET + LLDP_SYSTEM_DESCR_TLV_LEN_LEN) //46 + uIPLength
#define LLDP_SYSTEM_DESCR_TLV_LEN                3  // Can either be TLF or BSP

#define LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET       (LLDP_SYSTEM_DESCR_TLV_OFFSET + LLDP_SYSTEM_DESCR_TLV_LEN) // 49 + uIPLength
#define LLDP_END_OF_LLDPDU_TLV_TYPE_LEN          1
#define LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET        (LLDP_END_OF_LLDPDU_TLV_TYPE_OFFSET + LLDP_END_OF_LLDPDU_TLV_TYPE_LEN) //50 + uIPLength
#define LLDP_END_OF_LLDPDU_TLV_LEN_LEN           1
#define LLDP_END_OF_LLDPDU_TLV_OFFSET            (LLDP_END_OF_LLDPDU_TLV_LEN_OFFSET + LLDP_END_OF_LLDPDU_TLV_LEN_LEN) //51 + uIPLength

#define LLDP_MIN_BUFFER_SIZE    72
#define LLDP_MAX_BUFFER_SIZE    1024
volatile u8 uIPAddr[4];  // for IP address before dot notation 
volatile char pIPBuffer[15]; // for IP address string in dot notation 

enum LLDP_TLV_TYPE {

	/* start to mandatory TLV */
	LLDP_END_OF_LLDPDU_TLV = 0,
	LLDP_CHASSIS_ID_TLV = 2,
	LLPD_PORT_ID_TLV = 4,
	LLDP_TTL_TLV = 6,
	/* end of mandatory TLV */

	/* start of optional TLV */
	LLDP_PORT_DESC_TLV = 8,
	LLDP_SYSTEM_DESCR_TLV = 12,

};

enum LLDP_CHASSIS_ID_SUBTYPE {
	LLDP_CHASSIS_ID_MAC_ADDRESS = 4
};

enum LLDP_PORT_ID_SUBTYPE {
	LLDP_PORT_ID_NETWORK_ADDRESS = 4
};

int uLLDPBuildPacket(u8 uId, u8 *pTransmitBuffer, u32 *uResponseLength);
int uIPToString(char* pIPBuffer, u8* pIPAddr);



#endif /* SRC_LLDP_H_ */

