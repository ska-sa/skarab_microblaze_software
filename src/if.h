#ifndef _IF_H_
#define _IF_H_

#define IF_MAGIC 0xAABBCCDD

struct sIFObject{
  u32 uIFMagic;

  /* Tx buffer - ie data the user will send over the network */
  u8 *pUserTxBufferPtr;
  u16 uUserTxBufferSize;
  /* Rx buffer - ie data the user will receive over the network */
  u8 *pUserRxBufferPtr;
  u16 uUserRxBufferSize;

  u16 uMsgSize;
  u32 uNumWordsRead;

  u8 arrIFAddrIP[4];
  u8 arrIFAddrMac[6];

  /* RX Packet Counters */
  u32 uRxTotal; /* total packets received */
    /* Filtered Ethernet Types we handle */
    u32 uRxEthArp;
      u32 uRxArpReply;
      u32 uRxArpRequest;
      u32 uRxArpConflict;
      u32 uRxArpInvalid;
    u32 uRxEthIp;
      u32 uRxIpChecksumErrors;  /* IP header checksum errors */
      /* IP Types we handle */
      u32 uRxIpIcmp;
        u32 uRxIcmpInvalid;   /* these are packets destined for us but failed validation (problems with length, type, etc.) */
      u32 uRxIpUdp;
        u32 uRxUdpChecksumErrors;  /* UDP checksum errors */
        /* UDP Types we handle */
        u32 uRxUdpCtrl;     /* control packets */
          /* TODO control validation */
        u32 uRxUdpDhcp;     /* dhcp reply */
          u32 uRxDhcpInvalid;   /* these are packets destined for us but failed validation (problems with xid, etc.). 
                                   These include dhcp server bcast replies meant for other nodes */
        u32 uRxUdpUnknown;  /* packets dropped at UDP layer (includes dhcp requests from other nodes) */
      u32 uRxIpUnknown;     /* packets dropped at IP layer */
    u32 uRxEthUnknown;      /* packets dropped at Ethernet layer */

  /* TX Packet Counters */
  u32 uTxTotal;   /* total packets sent */
    
  /* layer 3 counters */
  u32 uTxEthArpRequestOk;
  u32 uTxEthArpReplyOk;
  u32 uTxEthArpErr;
  u32 uTxEthLldpOk;
  u32 uTxEthLldpErr;

  /* layer 4 counters */
  u32 uTxIpIcmpReplyOk;
  u32 uTxIpIcmpReplyErr;
  u32 uTxIpIgmpOk;
  u32 uTxIpIgmpErr;

  /* layer 5 counters */
  u32 uTxUdpDhcpOk;
  u32 uTxUdpDhcpErr;
  u32 uTxUdpCtrlOk;
  u32 uTxUdpCtrlAck;
  u32 uTxUdpCtrlNack;
};

#endif