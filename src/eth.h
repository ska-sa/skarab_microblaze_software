/**----------------------------------------------------------------------------
*   FILE:       eth.h
*   BRIEF:      Ethernet header definitions
*
*   DATE:       OCT 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
*------------------------------------------------------------------------------*/

#ifndef _ETH_H_
#define _ETH_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ETH_FRAME_BASE              0
#define ETH_DST_OFFSET              0
#define ETH_DST_LEN                 6
#define ETH_SRC_OFFSET              (ETH_DST_OFFSET + ETH_DST_LEN)  //6
#define ETH_SRC_LEN                 6
#define ETH_FRAME_TYPE_OFFSET       (ETH_SRC_OFFSET + ETH_SRC_LEN)  //12
#define ETH_FRAME_TYPE_LEN          2

#define ETH_FRAME_TOTAL_LEN         (ETH_FRAME_TYPE_OFFSET + ETH_FRAME_TYPE_LEN)  /* ethernet length = 14 */

#ifdef __cplusplus
}
#endif
#endif
