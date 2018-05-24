
#ifndef ETH_SORTER_H_
#define ETH_SORTER_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : eth_sorter.h
* ------------------------------------------------------------------------------
*  COMPANY              : PERALEX ELECTRONIC (PTY) LTD
* ------------------------------------------------------------------------------
*  COPYRIGHT NOTICE :
*
*  The copyright, manufacturing and patent rights stemming from this
*  document in any form are vested in PERALEX ELECTRONICS (PTY) LTD.
*
*  (c) Peralex 2011
*
*  PERALEX ELECTRONICS (PTY) LTD has ceded these rights to its clients
*  where contractually agreed.
* ------------------------------------------------------------------------------
*  DESCRIPTION :
*
*  This file contains the definitions for sorting Ethernet packets.
* ------------------------------------------------------------------------------*/

#include <xil_types.h>

#include "if.h"

u32 CalculateIPChecksum(u32 uChecksum, u32 uLength, u16 *pHeaderPtr);
int CheckIPV4Header(u32 uIPAddress, u32 uSubnet, u32 uPacketLength, u8 * pIPHeaderPointer);
u8 * ExtractIPV4FieldsAndGetPayloadPointer(u8 *pIPHeaderPointer, u32 *uIPPayloadLength, u32 *uResponseIPAddr, u32 *uProtocol, u32 * uTOS);
int CheckUdpHeader(u8 *pIPHeaderPointer, u32 uIPPayloadLength, u8 *pUdpHeaderPointer);
u8 *ExtractUdpFieldsAndGetPayloadPointer(u8 *pUdpHeaderPointer,u32 *uPayloadLength, u32 *uSourcePort, u32 *uDestinationPort);
int CommandSorter(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength, struct sIFObject *pIFObj);
int CheckCommandPacket(u8 * pCommand, u32 uCommandLength);
void CreateResponsePacket(u8 uId, u8 * uResponsePacketPtr, u32 uResponseLength);
int CheckArpRequest(u8 uId, u32 uFabricIPAddress, u32 uPktLen, u8 *pArpPacket);
void ArpHandler(u8 uId, u8 uType, u8 *pReceivedArp, u8 *pTransmitBuffer, u32 * uResponseLength, u32 uRequestedIPAddress);
void CreateIGMPPacket(u8 uId, u8 *pTransmitBuffer, u32 * uResponseLength, u8 uMessageType, u32 uGroupAddress);

// COMMAND HANDLERS
int WriteRegCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ReadRegCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int WriteWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ReadWishboneCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int WriteI2CCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ReadI2CCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int SdramReconfigureCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ReadFlashWordsCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ProgramFlashWordsCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int EraseFlashBlockCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ReadSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ProgramSpiPageCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int EraseSpiSectorCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int OneWireReadRomCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int OneWireDS2433WriteMemCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int OneWireDS2433ReadMemCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int DebugConfigureEthernetCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int DebugAddARPCacheEntryCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int GetEmbeddedSoftwareVersionCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int PMBusReadI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int ConfigureMulticastCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int DebugLoopbackTestCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int QSFPResetAndProgramCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int HMCReadI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int HMCWriteI2CBytesCommandHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int SDRAMProgramOverWishboneCommandHandler(u8 uId, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int SetDHCPTuningDebugCommandHandler(struct sIFObject *pIFObj, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int GetDHCPTuningDebugCommandHandler(struct sIFObject *pIFObj, u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

#endif
