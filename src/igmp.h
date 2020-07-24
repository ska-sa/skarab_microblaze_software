/*
 *  RvW (SARAO) - Dec 2019
 */

#ifndef _IGMP_H_
#define _IGMP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  IGMP_IDLE_STATE=0,
  IGMP_SEND_MEMBERSHIP_REPORTS_STATE,
  IGMP_JOINED_STATE,
  IGMP_LEAVING_STATE
} typeIGMPState;

struct sIGMPObject {
  u32 uIGMPMagic;

  typeIGMPState tIGMPCurrentState;

  u32 uIGMPCurrentMessage;
  u32 uIGMPCurrentClkTick;     /* internal timer tick */
  u8 uIGMPIfId;               /* interface id associated with this igmp context */

  u32 uIGMPJoinMulticastAddress;      /* base multicast address */
  u32 uIGMPJoinMulticastAddressMask;  /* mask to be applied to the base multicast addr above */

  u32 uIGMPLeaveMulticastAddress;
  u32 uIGMPLeaveMulticastAddressMask;

  volatile u8 uIGMPJoinRequestFlag;
  volatile u8 uIGMPLeaveRequestFlag;
};

struct sIGMPObject *pIGMPInit(u8 uId);

u8 uIGMPJoinGroup(u8 uId, u32 uMulticaseBaseAddr, u32 uMulticastAddrMask);
u8 uIGMPLeaveGroup(u8 uId);
u8 uIGMPLeaveGroupFlush(u8 uId);    /* blocks till all leave requests have been sent */
u8 uIGMPRejoinPrevGroup(u8 uId);

u8 uIGMPStateMachine(u8 uId);

void vIGMPPrintInfo(void);
#ifdef __cplusplus
}
#endif

#endif /*_IGMP_H_*/
