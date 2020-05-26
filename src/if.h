#ifndef _IF_H_
#define _IF_H_

#include "dhcp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IF_MAGIC 0xAABBCCDD

#define IF_RETURN_OK    (0)
#define IF_RETURN_FAIL  (1)

#define IF_RX_MIN_BUFFER_SIZE 1500   /*mtu*/
#define IF_TX_MIN_BUFFER_SIZE 1024

  /*TODO: note the order of the following may introduce stuct padding - perhaps
   * reorder for better ram usage */
struct sIFObject{
  u32 uIFMagic;

  /* Tx buffer - ie data the user will send over the network */
  u8 *pUserTxBufferPtr;
  u16 uUserTxBufferSize;
  u16 uMsgSize;   /* transmit message size built in the transmit buffer */

  /* Rx buffer - ie data the user will receive over the network */
  u8 *pUserRxBufferPtr;
  u16 uUserRxBufferSize;
  u32 uNumWordsRead;  /* number of words read into the receive buffer */

  u8 uIFLinkStatus;
  u8 uIFLinkRxActive;

  u8 arrIFAddrMac[6];

  u8 stringHostname[16];

  u8 arrIFAddrIP[4];
  u8 stringIFAddrIP[16];
  u32 uIFAddrIP;

  u8 arrIFAddrNetmask[4];
  u8 stringIFAddrNetmask[16];
  u32 uIFAddrMask;

  u8 uIFEthernetId;
  u32 uIFEthernetSubnet;

  struct sDHCPObject DHCPContextState;    /* holds the dhcp states for this interface */
//  struct sICMPObject ICMPContextState;    /* holds the icmp states for this interface */

  u8 uIFEnableArpRequests;  /* enable arp requests on this interface */
  u8 uIFCurrentArpRequest;  /* keep a counter to cycle through arp request IP's */

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
          u32 uRxDhcpUnknown;   /* possible dhcp broadcasts from other nodes destined for the dhcp server */
        u32 uRxUdpUnknown;  /* packets dropped at UDP layer */
      u32 uRxIpUnknown;     /* packets dropped at IP layer */
      u32 uRxIpPim;
        u32 uRxIpPimDropped;
      u32 uRxIpIgmp;
        u32 uRxIpIgmpDropped;
      u32 uRxIpTcp;
        u32 uRxIpTcpDropped;
    u32 uRxEthLldp;         /* lldp packets */
      u32 uRxEthLldpDropped;
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

struct sIFObject *InterfaceInit(u8 uEthernetId, u8 *pRxBufferPtr, u16 uRxBufferSize, u8 *pTxBufferPtr, u16 uTxBufferSize, u8 *arrUserMacAddr);

u8 if_enumerate_interfaces(void);   /* returns number of interfaces */
u8 get_num_interfaces(void);
u8 get_interface_id(u8 logical_if_id);
u8 check_interface_valid(u8 physical_interface_id);

u8 *if_generate_hostname_string(u8 phy_interface_id);     /* arg is the physical interface id/position */
u8 *if_generate_mac_addr_array(u8 phy_interface_id);

struct sIFObject *lookup_if_handle_by_id(u8 id);

void IFConfig(struct sIFObject *pIFObjectPtr, u32 ip, u32 mask);
void UpdateEthernetLinkUpStatus(struct sIFObject *pIFObjectPtr);


typedef enum {
  /* these are enumerations for protocols we are expecting */
  PACKET_FILTER_ARP,
  PACKET_FILTER_ICMP,
  PACKET_FILTER_DHCP,
  PACKET_FILTER_CONTROL,

  /* known but unhandled / dropped protocols */
  PACKET_FILTER_IGMP_UNHANDLED,
  PACKET_FILTER_PIM_UNHANDLED,
  PACKET_FILTER_TCP_UNHANDLED,
  PACKET_FILTER_LLDP_UNHANDLED,

  /* these are enumerations to handle unexpected packets or errors */
  PACKET_FILTER_UNKNOWN,
  PACKET_FILTER_ERROR,
  PACKET_FILTER_DROP,
  PACKET_FILTER_UNKNOWN_UDP,
  PACKET_FILTER_UNKNOWN_IP,
  PACKET_FILTER_UNKNOWN_ETH
} typePacketFilter;

#define ETHER_TYPE_ARP    0x0806
#define ETHER_TYPE_IPV4   0x0800
#define ETHER_TYPE_LLDP   0x88cc

#define IPV4_TYPE_ICMP   0x0001
#define IPV4_TYPE_IGMP   0x0002
#define IPV4_TYPE_UDP    0x0011
#define IPV4_TYPE_TCP    0x0006
#define IPV4_TYPE_PIM    0x0067

#define UDP_CONTROL_PORT  0x7778
#define BOOTP_CLIENT_PORT 0x44
#define BOOTP_SERVER_PORT 0x43

typePacketFilter uRecvPacketFilter(struct sIFObject *pIFObjectPtr);

typedef enum {
  RX_TOTAL,
  RX_ETH_ARP,
  RX_ARP_REPLY,
  RX_ARP_REQUEST,
  RX_ARP_CONFLICT,
  RX_ARP_INVALID,
  RX_ETH_IP,
  RX_IP_CHK_ERR,
  RX_IP_ICMP,
  RX_ICMP_INVALID,
  RX_IP_UDP,
  RX_UDP_CHK_ERR,
  RX_UDP_CTRL,
  RX_UDP_DHCP,
  RX_DHCP_INVALID,
  RX_DHCP_UNKNOWN,
  RX_UDP_UNKNOWN,
  RX_IP_UNKNOWN,
  RX_ETH_UNKNOWN,
  TX_TOTAL,
  TX_ETH_ARP_REQ_OK,
  TX_ETH_ARP_REPLY_OK,
  TX_ETH_ARP_ERR,
  TX_ETH_LLDP_OK,
  TX_ETH_LLDP_ERR,
  TX_IP_ICMP_REPLY_OK,
  TX_IP_ICMP_REPLY_ERR,
  TX_IP_IGMP_OK,
  TX_IP_IGMP_ERR,
  TX_UDP_DHCP_OK,
  TX_UDP_DHCP_ERR,
  TX_UDP_CTRL_OK,
  TX_UDP_CTRL_ACK,
  TX_UDP_CTRL_NACK
} tCounter;

void IFCounterIncr(struct sIFObject *pIFObjectPtr, tCounter c);

#ifdef __cplusplus
}
#endif
#endif
