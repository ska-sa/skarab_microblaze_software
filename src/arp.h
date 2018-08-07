/**----------------------------------------------------------------------------
*   FILE:       arp.h
*   BRIEF:      ARP protocol definitions.
*
*   DATE:       OCT 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
*------------------------------------------------------------------------------*/

#ifndef _ARP_H_
#define _ARP_H_

#include <xstatus.h>

#include "eth.h"

#ifdef __cplusplus
extern "C" {
#endif

/* define return values */
#ifndef ARP_RETURN_OK
#define ARP_RETURN_OK (0)
#endif

#ifndef ARP_RETURN_FAIL_
#define ARP_RETURN_FAIL  (1)
#endif

#ifndef ARP_RETURN_REQUEST
#define ARP_RETURN_REQUEST  (2)
#endif

#ifndef ARP_RETURN_REPLY
#define ARP_RETURN_REPLY (3)
#endif

#ifndef ARP_RETURN_INVALID
#define ARP_RETURN_INVALID (4)
#endif

#ifndef ARP_RETURN_CONFLICT
#define ARP_RETURN_CONFLICT (5)
#endif

#ifndef ARP_RETURN_IGNORE
#define ARP_RETURN_IGNORE (6)
#endif

#define ARP_FRAME_BASE                (ETH_FRAME_BASE + ETH_FRAME_TOTAL_LEN)
#define ARP_HW_TYPE_OFFSET            0
#define ARP_HW_TYPE_LEN               2
#define ARP_PROTO_TYPE_OFFSET         (ARP_HW_TYPE_OFFSET + ARP_HW_TYPE_LEN)
#define ARP_PROTO_TYPE_LEN            2
#define ARP_HW_ADDR_LENGTH_OFFSET     (ARP_PROTO_TYPE_OFFSET + ARP_PROTO_TYPE_LEN)
#define ARP_HW_ADDR_LENGTH_LEN        1
#define ARP_PROTO_ADDR_LENGTH_OFFSET  (ARP_HW_ADDR_LENGTH_OFFSET + ARP_HW_ADDR_LENGTH_LEN)
#define ARP_PROTO_ADDR_LENGTH_LEN     1
#define ARP_OPCODE_OFFSET             (ARP_PROTO_ADDR_LENGTH_OFFSET + ARP_PROTO_ADDR_LENGTH_LEN)
#define ARP_OPCODE_LEN                2
/* NOTE: usually the length of following address fields are determined
         by the corresponding address length fields above, however,
         we're only expecting ipv4 over ethernet arp traffic */
#define ARP_SRC_HW_ADDR_OFFSET        ARP_OPCODE_OFFSET + ARP_OPCODE_LEN
#define ARP_SRC_HW_ADDR_LEN           6
#define ARP_SRC_PROTO_ADDR_OFFSET     ARP_SRC_HW_ADDR_OFFSET + ARP_SRC_HW_ADDR_LEN
#define ARP_SRC_PROTO_ADDR_LEN        4
#define ARP_TGT_HW_ADDR_OFFSET        ARP_SRC_PROTO_ADDR_OFFSET + ARP_SRC_PROTO_ADDR_LEN
#define ARP_TGT_HW_ADDR_LEN           6
#define ARP_TGT_PROTO_ADDR_OFFSET     ARP_TGT_HW_ADDR_OFFSET + ARP_TGT_HW_ADDR_LEN
#define ARP_TGT_PROTO_ADDR_LEN        4

#define ARP_FRAME_TOTAL_LEN           (ARP_TGT_PROTO_ADDR_OFFSET + ARP_TGT_PROTO_ADDR_LEN)

typedef enum {ARP_OPCODE_REQUEST=1,
              ARP_OPCODE_REPLY
} typeARPMessage;

struct sIFObject;

u8 uARPMessageValidateReply(struct sIFObject *pIFObjectPtr);
u8 uARPBuildMessage(struct sIFObject *pIFObjectPtr, typeARPMessage tARPMsgType, u32 uTargetIP);
void ArpRequestHandler(struct sIFObject *pIFObjectPtr);

#ifdef __cplusplus
}
#endif
#endif
