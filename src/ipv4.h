/**----------------------------------------------------------------------------
*   FILE:       ipv4.h
*   BRIEF:      IP v4 definitions
*
*   DATE:       OCT 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
*------------------------------------------------------------------------------*/

#ifndef _IPV4_H_
#define _IPV4_H_

#include "eth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IP_FRAME_BASE               (ETH_FRAME_BASE + ETH_FRAME_TOTAL_LEN)
#define IP_V_HIL_OFFSET             0
#define IP_V_HIL_LEN                1
#define IP_TOS_OFFSET               (IP_V_HIL_OFFSET + IP_V_HIL_LEN)  //1
#define IP_TOS_LEN                  1
#define IP_TLEN_OFFSET              (IP_TOS_OFFSET + IP_TOS_LEN)  //2
#define IP_TLEN_LEN                 2
#define IP_ID_OFFSET                (IP_TLEN_OFFSET + IP_TLEN_LEN) //4
#define IP_ID_LEN                   2
#define IP_FLAG_FRAG_OFFSET         (IP_ID_OFFSET + IP_ID_LEN) //6
#define IP_FLAG_FRAG_LEN            2
#define IP_TTL_OFFSET               (IP_FLAG_FRAG_OFFSET + IP_FLAG_FRAG_LEN) //8
#define IP_TTL_LEN                  1
#define IP_PROT_OFFSET              (IP_TTL_OFFSET + IP_TTL_LEN) //9
#define IP_PROT_LEN                 1
#define IP_CHKSM_OFFSET             (IP_PROT_OFFSET + IP_PROT_LEN) //10
#define IP_CHKSM_LEN                2
#define IP_SRC_OFFSET               (IP_CHKSM_OFFSET + IP_CHKSM_LEN)//12
#define IP_SRC_LEN                  4
#define IP_DST_OFFSET               (IP_SRC_OFFSET + IP_SRC_LEN)//16
#define IP_DST_LEN                  4

#define IP_FRAME_TOTAL_LEN          (IP_DST_OFFSET + IP_DST_LEN)  /* ip length = 20 */

#ifdef __cplusplus
}
#endif
#endif
