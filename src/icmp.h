/**----------------------------------------------------------------------------
*   FILE:       icmp.h
*   BRIEF:      ICMP declarations and API.
*
*   DATE:       OCT 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   NOTES:      vim settings: "set sw=2 ts=2 expandtab autoindent"
*------------------------------------------------------------------------------*/

#ifndef _ICMP_H_
#define _ICMP_H_

/* Xilinx lib includes */
#include <xstatus.h>

#include "eth.h"
#include "ipv4.h"

/* link custom return values */
#define ICMP_RETURN_OK            XST_SUCCESS
#define ICMP_RETURN_FAIL          XST_FAILURE
#define ICMP_RETURN_NOT_ENABLED   XST_NOT_ENABLED
#define ICMP_RETURN_INVALID       XST_FAILURE

/* define return values */
#ifndef ICMP_RETURN_OK
#define ICMP_RETURN_OK (0)
#endif

#ifndef ICMP_RETURN_FAIL
#define ICMP_RETURN_FAIL  (1)
#endif

#ifndef ICMP_RETURN_NOT_ENABLED
#define ICMP_RETURN_NOT_ENABLED (2)
#endif

#ifndef ICMP_RETURN_INVALID
#define ICMP_RETURN_INVALID (3)
#endif

#define ICMP_FRAME_BASE             (IP_FRAME_BASE + IP_FRAME_TOTAL_LEN)
#define ICMP_TYPE_OFFSET            0
#define ICMP_TYPE_LEN               1
#define ICMP_CODE_OFFSET            (ICMP_TYPE_OFFSET + ICMP_TYPE_LEN)
#define ICMP_CODE_LEN               1
#define ICMP_CHKSM_OFFSET           (ICMP_CODE_OFFSET + ICMP_CODE_LEN)
#define ICMP_CHKSM_LEN              2
#define ICMP_DATA_OFFSET            (ICMP_CHKSM_OFFSET + ICMP_CHKSM_LEN)

#define ICMP_MAGIC  0xBADC0C0A

struct sICMPObject{
  u32 uICMPMagic;

  /* Tx buffer - ie data the user will send over the network */
  u8 *pUserTxBufferPtr;
  u16 uUserTxBufferSize;

  /* Rx buffer - ie data the user will receive over the network */
  u8 *pUserRxBufferPtr;
  u16 uUserRxBufferSize;

  u16 uICMPMsgSize;
};

u8 uICMPInit(struct sICMPObject *pICMPObjectPtr, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize);
u8 uICMPMessageValidate(struct sICMPObject *pICMPObjectPtr);
u8 uICMPBuildReplyMessage(struct sICMPObject *pICMPObjectPtr);

#endif
