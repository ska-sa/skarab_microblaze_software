/**---------------------------------------------------------------------------- 
*   FILE:       dhcp.h
*   BRIEF:      API of DHCP client functionality to obtain and renew IPv4 leases.
*
*   DATE:       MAY 2017
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*   REFERENCES: RFC 2131 & 2132
*------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

#ifndef _DHCP_H_
#define _DHCP_H_

/* Xilinx lib includes */
#include <xstatus.h>
#include <xil_types.h>

//#include "if.h"
//#include "eth.h"
//#include "ipv4.h"
#include "udp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* link custom return values */
#define DHCP_RETURN_OK            XST_SUCCESS
#define DHCP_RETURN_FAIL          XST_FAILURE
#define DHCP_RETURN_NOT_ENABLED   XST_NOT_ENABLED
#define DHCP_RETURN_INVALID       XST_FAILURE

/* define return values */
#ifndef DHCP_RETURN_OK
#define DHCP_RETURN_OK (0)
#endif

#ifndef DHCP_RETURN_FAIL
#define DHCP_RETURN_FAIL  (1)
#endif

#ifndef DHCP_RETURN_NOT_ENABLED
#define DHCP_RETURN_NOT_ENABLED (2)
#endif

#ifndef DHCP_RETURN_INVALID
#define DHCP_RETURN_INVALID (3)
#endif

#define DHCP_RETURN_FALSE 0
#define DHCP_RETURN_TRUE  1

/* dhcp packet offsets */

#define BOOTP_FRAME_BASE            (UDP_FRAME_BASE + UDP_HEADER_TOTAL_LEN) //42
#define BOOTP_OPTYPE_OFFSET         0
#define BOOTP_OPTYPE_LEN            1
#define BOOTP_HWTYPE_OFFSET         (BOOTP_OPTYPE_OFFSET + BOOTP_OPTYPE_LEN) //1
#define BOOTP_HWTYPE_LEN            1
#define BOOTP_HWLEN_OFFSET          (BOOTP_HWTYPE_OFFSET + BOOTP_HWTYPE_LEN) //2
#define BOOTP_HWLEN_LEN             1
#define BOOTP_HOPS_OFFSET           (BOOTP_HWLEN_OFFSET + BOOTP_HWLEN_LEN) //3
#define BOOTP_HOPS_LEN              1
#define BOOTP_XID_OFFSET            (BOOTP_HOPS_OFFSET + BOOTP_HOPS_LEN) //4
#define BOOTP_XID_LEN               4
#define BOOTP_SEC_OFFSET            (BOOTP_XID_OFFSET + BOOTP_XID_LEN) //8
#define BOOTP_SEC_LEN               2
#define BOOTP_FLAGS_OFFSET          (BOOTP_SEC_OFFSET + BOOTP_SEC_LEN) //10
#define BOOTP_FLAGS_LEN             2
#define BOOTP_CIPADDR_OFFSET        (BOOTP_FLAGS_OFFSET + BOOTP_FLAGS_LEN) //12
#define BOOTP_CIPADDR_LEN           4
#define BOOTP_YIPADDR_OFFSET        (BOOTP_CIPADDR_OFFSET + BOOTP_CIPADDR_LEN) //16
#define BOOTP_YIPADDR_LEN           4
#define BOOTP_SIPADDR_OFFSET        (BOOTP_YIPADDR_OFFSET + BOOTP_YIPADDR_LEN) //20
#define BOOTP_SIPADDR_LEN           4
#define BOOTP_GIPADDR_OFFSET        (BOOTP_SIPADDR_OFFSET + BOOTP_SIPADDR_LEN) //24
#define BOOTP_GIPADDR_LEN           4
#define BOOTP_CHWADDR_OFFSET        (BOOTP_GIPADDR_OFFSET + BOOTP_GIPADDR_LEN) //28
#define BOOTP_CHWADDR_LEN           16
#define BOOTP_SNAME_OFFSET          (BOOTP_CHWADDR_OFFSET + BOOTP_CHWADDR_LEN)//44
#define BOOTP_SNAME_LEN             64
#define BOOTP_FILE_OFFSET           (BOOTP_SNAME_OFFSET + BOOTP_SNAME_LEN)//108
#define BOOTP_FILE_LEN              128
#define BOOTP_OPTIONS_OFFSET        (BOOTP_FILE_OFFSET + BOOTP_FILE_LEN) //236   //start of the dhcp options

#define BOOTP_FRAME_TOTAL_LEN       BOOTP_OPTIONS_OFFSET  /* bootp length = 236 */

#define DHCP_OPTIONS_BASE           (BOOTP_FRAME_BASE + BOOTP_OPTIONS_OFFSET)

/* two mechanisms when message ready:
    1) TODO polling - poll the uDHCPMessageReady flag
    2) callback - call a user provided function which 
       knows what to do with the user provided buffer*/

#define DHCP_SM_RETRIES   5
#define DHCP_SM_INTERVAL  50    /* interval time (ms) = POLL_INTERVAL * DHCP_SM_INTERVAL */
                                /* Retry rate -> The state machine will reset if this interval expires before a lease is obtained. */
#define POLL_INTERVAL 100       /* polling interval, in msecs. ie interval at which state machine is invoked - user defined */
#define DHCP_SM_WAIT  40        /* wait a prerequisite amount of time before starting dhcp. */
                                /* -> times by POLL_INTERVAL to get milli-seconds value */
                                /* NOTE: must be smaller than DHCP_SM_INTERVAL */

/* 14 [eth] + 20 [ip] + 8 [udp] + 236 [bootp] + 312 [min dhcp option length - rfc 2131, pg. 10] */
#define DHCP_MIN_BUFFER_SIZE  590  /*FIXME*/
#define HOST_NAME_MAX_SIZE    16    /* allows for hostname size of 15 characters plus '\0' */

/*prefix: arr = array, t = enumerated type, p = pointer, u = unsigned data type, i = integer, s = struct*/

/* versioning macros */
#ifndef GITVERSION
#define GITVERSION "unknown-state"
#endif

#ifndef GITVERSION_SIZE
#define GITVERSION_SIZE 13
#endif

#define VENDOR_ID (GITVERSION" "__DATE__" "__TIME__)    /* build a string */
#define VENDOR_ID_LEN (21 + (GITVERSION_SIZE))  /* length of the above string */
#define STRING(x) XSTRING(x)
#define XSTRING(x) #x

typedef enum {INIT=0,
              RANDOMIZE,
              SELECT,
              WAIT,
              REQUEST,
              BOUND,
              RENEW,
              REBIND
} typeDHCPState;

typedef enum {DHCPDISCOVER=1,
              DHCPOFFER,
              DHCPREQUEST,
              DHCPDECLINE,
              DHCPACK,
              DHCPNAK,
              DHCPRELEASE,
              DHCPINFORM,
              DHCPERROR = 0xff    /* custom addition to return errors */
} typeDHCPMessage;

typedef enum {flagDHCP_SM_AUTO_REDISCOVER=0,
              flagDHCP_SM_LEASE_OBTAINED,
              flagDHCP_SM_GOT_MESSAGE,
              flagDHCP_SM_STATE_MACHINE_EN,
              flagDHCP_MSG_REQ_HOST_NAME,
              flagDHCP_SM_SHORT_CIRCUIT_RENEW,
              /*
               * flagDHCP_SM_SHORT_CIRCUIT_RENEW:
               * The next flag is used to short circuit the renewal step and
               * move back to the discover step.
               * This was implemented to circumvent the case where a relay agent
               * has appended tags during the initial lease acquisition step and
               * these tags were used to issue the lease. Upon renewal, the
               * client unicasts a message directly to the dhcp server (in line
               * with RFC 2131) thus bypassing the relay agent and in turn the
               * tags are not appended. This may cause issues resulting in the
               * lease not being successfully renewed.  This flag bypasses the
               * renew step altogether, and allows the client to re-discover the
               * lease.
               */
              flagDHCP_SM_INIT_WAIT_EN,
              /*
               * flagDHCP_SM_INIT_WAIT_EN:
               * this flag determines if the state machine must wait a
               * fixed amount of time (DHCP_SM_WAIT) at init before running
               */
              flagDHCP_SM_USE_CACHED_IP
                /*
                 * flagDHCP_SM_USE_CACHED_IP:
                 * this flag was added to allow the state machine to try and
                 * re-acquire the ip it previously held. This has the potential
                 * to "half" the DHCP transactions by sending out a request
                 * message with this "cached" ip, thereby skipping the discover
                 * step
                 */
} typeDHCPRegisterFlags;

typedef enum {flagDHCP_MSG_UNICAST=0,
              flagDHCP_MSG_BOOTP_CIPADDR,
              flagDHCP_MSG_DHCP_REQIP,
              flagDHCP_MSG_DHCP_SVRID,
              flagDHCP_MSG_DHCP_REQPARAM,
              flagDHCP_MSG_DHCP_VENDID,
              flagDHCP_MSG_DHCP_NEW_XID,
              flagDHCP_MSG_DHCP_RESET_SEC
} typeDHCPMessageFlags;

typedef enum {statusCLR=0,
              statusSET=1,
              statusERR
} typeDHCPFlagStatus;

/* sDHCPObject magic value */
#define DHCP_MAGIC 0x1C0FFEE1

struct sIFObject;

typedef typeDHCPState (* const dhcp_state_func_ptr)(struct sIFObject *pIFObjectPtr);

/* callback type definition - user function form */
typedef int (*tcallUserFunction)(struct sIFObject *pIFObjectPtr, void *pUserData);

#define SM_FALSE  0
#define SM_TRUE   1

/*
 * maximum amount of DHCPRequest messages
 * to send after reboot when attempting to
 * re-acquire previous lease (cached IP)
 */
#define DHCP_REBOOTING_REQS_MAX 10

/* object which holds the current context/state */
struct sDHCPObject{
  u32 uDHCPMagic;

  tcallUserFunction callbackOnMsgBuilt;
  void *pMsgDataPtr;

  /* set the flagDHCP_SM_GOT_MESSAGE flag when there's a message to process */

  /* DHCP configuration */
  u8  arrDHCPAddrYIPCached[4];
  u8  arrDHCPAddrSubnetMask[4];
  u8  arrDHCPAddrRoute[4];
  tcallUserFunction callbackOnLeaseAcqd;
  void *pLeaseDataPtr;

  /* useful states and variables */
  u8  uDHCPMessageReady;
  u8  uDHCPTx;
  u8  uDHCPRx;
  u8  uDHCPErrors;
  u8  uDHCPInvalid;
  u8  uDHCPRetries;
  u8  uDHCPTimeoutStatus;   /* indicates that the DHCP process has failed after
                             * <DHCP_SM_RETRIES> attempts */

  u8  uDHCPRebootReqs;      /* count of the requests sent at reboot if using a
                             * cached IP to attempt to re-aquire a previous
                             * lease */

  /* internal states and variables */
  u16 uDHCPTimeout;         /* counter used to timeout if "stuck" in a state */
  typeDHCPState tDHCPCurrentState;     /* holds state of dhcp state machine */

  u32 uDHCPInternalTimer;
  u32 uDHCPCurrentClkTick;
  u32 uDHCPCachedClkTick;
  u32 uDHCPExternalTimerTick;
  u32 uDHCPRandomWait;
  u32 uDHCPRandomWaitCached;

  u32 uDHCPInitUnboundTimer;      /* timer to keep track of total time that the dhcp has been unresolved at startup */

  u32 uDHCPSMRetryInterval;   /* time between dhcp retries */
  u32 uDHCPSMInitWait;        /* time to wait before sending out first dhcp discover packet */

  u32 uDHCPT1;
  u32 uDHCPT2;
  u32 uDHCPLeaseTime;

  /* byte arrays */
#define DHCP_MAC_ARR_LEN  6
  u8  arrDHCPNextHopMacCached[DHCP_MAC_ARR_LEN];       /* holds the mac address of the next hop on link */
#define DHCP_IPADDR_ARR_LEN  4
  u8  arrDHCPAddrServerCached[DHCP_IPADDR_ARR_LEN];       /* holds the lease issuing dhcp server's ip address */

  u32 uDHCPXidCached;

  u8  arrDHCPHostName[HOST_NAME_MAX_SIZE];        /* array which holds the host name */

  u8  uDHCPRegisterFlags;         /* Register which holds DHCP status flags */
};


/*TODO: create and destroy functions - to use if there is sufficient heap space */
#if 0
/* create on heap */
struct sDHCPObject *pDHCPCreateObjHandle();

/* free off heap */
void xDHCPDestroyObjHandle(struct sDHCPObject *pDHCPObjectPtr);
#endif

/* required - call this function first to initialize DHCP */
/* NB - the referenced Rx and Tx buffer space must persist for as long as the sDHCPObject is valid */
u8 uDHCPInit(struct sIFObject *pIFObjectPtr);

/* required - invoke the DHCP state machine - typically called on a timer interrupt */
u8 uDHCPStateMachine(struct sIFObject *pIFObjectPtr);

/* configuration API / wrappers*/
/* required -  */
u8 uDHCPSetStateMachineEnable(struct sIFObject *pIFObjectPtr, u8 uEnable);

u8 vDHCPStateMachineReset(struct sIFObject *pIFObjectPtr);

/* delay the start of dhcp - see DHCP_SM_WAIT macro above */
u8 uDHCPSetWaitOnInitFlag(struct sIFObject *pIFObjectPtr);
u8 uDHCPSetInitWait(struct sIFObject *pIFObjectPtr, u32 uInitWait);
u8 uDHCPSetRetryInterval(struct sIFObject *pIFObjectPtr, u32 uRetryInterval);

/*
 * request a specific lease on startup - in line with INIT-REBOOTING state of
 * RFC 2131.
 */
u8 uDHCPSetRequestCachedIP(struct sIFObject *pIFObjectPtr, u32 uCachedIP);

#if 0
/*TODO*/u8 uDHCPSetAutoRediscoverEnable(struct sDHCPObject *pDHCPObjectPtr, u8 uEnable);
#endif

/* The following two functions are usually used in conjunction with each other...*/
/* first... check if the user rx buffer contains a valid DHCP packet */
u8 uDHCPMessageValidate(struct sIFObject *pIFObjectPtr);

/* then... set flag once there's a valid message in the user Rx buffer */
u8 uDHCPSetGotMsgFlag(struct sIFObject *pIFObjectPtr);

/* u8 uDHCPGetTimeoutStatus(struct sDHCPObject *pDHCPObjectPtr); */

#if 0
/* TODO: the other option is to read the user RX buffer and wait for valid message */
/* TODO FIXME implement */u8 uDHCPSetAlwaysOnFlag(struct sDHCPObject *pDHCPObjectPtr);
/* poll the status of the DHCP message built in user tx buffer, to be sent via interface*/
/* TODO */ u8 uDHCPPollMsgReady(struct sDHCPObject *pDHCPObjectPtr);
#endif

/* set the host name to be used in message when requesting dhcp lease from server */
u8 vDHCPSetHostName(struct sIFObject *pIFObjectPtr, const char *stringHostName);

/* get the dhcp lease-binding status */
u8 uDHCPGetBoundStatus(struct sIFObject *pIFObjectPtr);

u32 uDHCPGetInitUnboundTime(struct sIFObject *pIFObjectPtr);

/* event-driven callbacks */

/* register a callback to be invoked once the DHCP message successfully built */
  /* this callback will typically be aware of the user Tx buffer and handle the i/o for it */
int eventDHCPOnMsgBuilt(struct sIFObject *pIFObjectPtr, tcallUserFunction callback, void *pDataPtr);

/* register a callback to be invoked  once the DHCP lease has been acquired*/
  /* this callback will typically set the interface configurations - ip, subnet, etc. */
int eventDHCPOnLeaseAcqd(struct sIFObject *pIFObjectPtr, tcallUserFunction callback, void *pDataPtr);

#ifdef __cplusplus
}
#endif
#endif /* _DHCP_H_ */
