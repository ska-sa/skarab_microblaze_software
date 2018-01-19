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

  u8 arrIFAddrIP[4];
  u8 arrIFAddrMac[6];
};

#endif
