#ifndef CONSTANT_DEFS_H_
#define CONSTANT_DEFS_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : constant_defs.h
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
*  This file contains global constant and type definitions.
* ------------------------------------------------------------------------------*/

#include "xtmrctr.h"
#include "xintc.h"

// GLOBAL VARIABLES

//#define SKARAB_BSP

#ifdef SKARAB_BSP
#define NUM_ETHERNET_INTERFACES			0x1
#define DO_1GBE_LOOPBACK_TEST
#else
#define NUM_ETHERNET_INTERFACES			0x2//AI: Single 40GbE Core 0x5
//#define DO_1GBE_LOOPBACK_TEST
#endif

//DEFINE INTERFACE NAMES

#ifndef ONE_GBE_INTERFACE
#define ONE_GBE_INTERFACE          "I/F-01GBE-00"
#endif
#ifndef FORTY_GBE_INTERFACE
#define FORTY_GBE_INTERFACE       "I/F-40GBE-01"
#endif

#define DEBUG_PRINT
//uncomment if you want to see more debug output
//#define TRACE_PRINT

//#define DO_40GBE_LOOPBACK_TEST

//#define DEV_PLATFORM

#define NUM_REGISTERS				32

volatile u32 uWriteBoardShadowRegs[NUM_REGISTERS];

// Single transmit buffer
volatile u32 uTransmitBuffer[256];

// Single receive buffer
volatile u32 uReceiveBuffer[256];

// Transmit and receive buffers for loopback testing of second interface
volatile u32 uLoopbackTransmitBuffer[256];
volatile u32 uLoopbackReceiveBuffer[256];

volatile u16 uResponseMacHigh;
volatile u16 uResponseMacMid;
volatile u16 uResponseMacLow;
volatile u32 uResponseIPAddr;
volatile u32 uResponseUDPPort;

volatile u16 uEthernetFabricMacHigh[NUM_ETHERNET_INTERFACES];
volatile u16 uEthernetFabricMacMid[NUM_ETHERNET_INTERFACES];
volatile u16 uEthernetFabricMacLow[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetFabricIPAddress[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetFabricSubnetMask[NUM_ETHERNET_INTERFACES];
volatile u16 uEthernetFabricPortAddress[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetFabricMultiCastIPAddress[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetFabricMultiCastIPAddressMask[NUM_ETHERNET_INTERFACES];
volatile u16 uIPIdentification[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetSubnet[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetGatewayIPAddress[NUM_ETHERNET_INTERFACES];

volatile u8 uCurrentArpEthernetInterface;
volatile u8 uUpdateArpRequests;
volatile u8 uEnableArpRequests[NUM_ETHERNET_INTERFACES];
volatile u8 uCurrentArpRequest;
volatile u8 uEthernetLinkUp[NUM_ETHERNET_INTERFACES];
volatile u8 uEthernetNeedsReset[NUM_ETHERNET_INTERFACES];

volatile u32 uLLDPTimerCounter;
volatile u8 uLLDPRetryTimer[NUM_ETHERNET_INTERFACES];

volatile u32 uDHCPTimerCounter;
volatile u32 uDHCPTransactionID[NUM_ETHERNET_INTERFACES];
volatile u8 uDHCPState[NUM_ETHERNET_INTERFACES];
volatile u8 uDHCPRetryTimer[NUM_ETHERNET_INTERFACES];

volatile u8 uIGMPTimerCounter;
volatile u8 uIGMPState[NUM_ETHERNET_INTERFACES];
volatile u8 uIGMPSendMessage[NUM_ETHERNET_INTERFACES];
volatile u8 uCurrentIGMPMessage[NUM_ETHERNET_INTERFACES];

volatile u8 uDoReboot;

volatile u8 uQSFPMezzanineLocation;
volatile u8 uQSFPMezzaninePresent;
volatile u32 uQSFPMezzanineCurrentTxLed;
volatile u32 uQSFPMezzanineCurrentRxLed;
volatile u32 uQSFPCtrlReg;
volatile u8 uQSFPUpdateStatusEnable;
volatile u8 uQSFPUpdateState;
volatile u8 uQSFPI2CMicroblazeAccess;
volatile u32 uQSFPStateCounter;
volatile u8 uQSFPState;

volatile u8 uFrontPanelLeds;
volatile u8 uFrontPanelLedsValue;
volatile u8 uDHCPCompleteSetLeds;
volatile u32 uFrontPanelTimerCounter;

// SPECIAL CODE TO HANDLE ONE OF PROTOTYPE SYSTEMS HAS FAULTY I2C ON MEZZANINE SITE 1
volatile u32 uPxSerialNumber;

volatile u16 uQSFPBootloaderVersionMajor;
volatile u16 uQSFPBootloaderVersionMinor;

volatile u32 uPreviousAsyncSdramRead;
volatile u16 uPreviousSequenceNumber;

/* <major>.<minor>.<patch> */
#define EMBEDDED_SOFTWARE_VERSION_MAJOR		3
#define EMBEDDED_SOFTWARE_VERSION_MINOR		1
#define EMBEDDED_SOFTWARE_VERSION_PATCH		2

// WISHBONE SLAVE ADDRESSES
#define BOARD_REGISTER_ADDR			0x00000000
#define FLASH_SDRAM_SPI_ICAPE_ADDR	0x00010000
#define ONE_WIRE_ADDR				0x00018000
#define I2C_0_ADDR					0x00020000
#define I2C_1_ADDR					0x00028000
#define I2C_2_ADDR					0x00030000
#define I2C_3_ADDR					0x00038000
#define I2C_4_ADDR					0x00040000
#define ONE_GBE_MAC_ADDR			0x00048000
#define FORTY_GBE_MAC_0_ADDR		0x00050000
#define FORTY_GBE_MAC_1_ADDR		0x00058000
#define FORTY_GBE_MAC_2_ADDR		0x00060000
#define FORTY_GBE_MAC_3_ADDR		0x00068000
#define DSP_REGISTER_ADDR			0x00070000

/*#define DUAL_PORT_RAM_ADDR			0x00000
#define BOARD_REGISTER_ADDR			0x08000
#define ONE_GBE_MAC_ADDR			0x10000
#define FORTY_GBE_MAC_0_ADDR		0x18000
#define FLASH_SDRAM_SPI_ICAPE_ADDR	0x20000
#define ONE_WIRE_ADDR				0x28000
#define I2C_0_ADDR					0x30000
#define I2C_1_ADDR					0x38000
#define I2C_2_ADDR					0x40000
#define DSP_REGISTER_ADDR			0xF8000*/

// BOARD REGISTER OFFSET
// READ REGISTERS
#define C_RD_VERSION_ADDR          0x0
#define C_RD_BRD_CTL_STAT_0_ADDR   0x4
#define C_RD_LOOPBACK_ADDR         0x8
#define C_RD_ETH_IF_LINK_UP_ADDR   0xC
#define C_RD_MEZZANINE_STAT_ADDR   0x10
#define C_RD_USB_STAT_ADDR         0x14
#define C_RD_FPGA_DNA_LOW_ADDR     0x1C
#define C_RD_FPGA_DNA_HIGH_ADDR    0x20

// WRITE REGISTERS
#define C_WR_BRD_CTL_STAT_0_ADDR   		0x4
#define C_WR_LOOPBACK_ADDR         		0x8
#define C_WR_ETH_IF_CTL_ADDR   			0xC
#define C_WR_MEZZANINE_CTL_ADDR			0x10
#define C_WR_FRONT_PANEL_STAT_LED_ADDR	0x14
#define C_WR_BRD_CTL_STAT_1_ADDR   		0x18

// DSP REGISTER ADDRESSES

#define ETHERNET_FABRIC_PORT_ADDRESS	0x7148
#define ETHERNET_CONTROL_PORT_ADDRESS	0x7778
#define ETHERNET_FABRIC_SOURCE_MAC_HIGH 0x000B
#define ETHERNET_FABRIC_SOURCE_MAC_MID	0x0E0F
#define ETHERNET_FABRIC_SOURCE_MAC_LOW	0x00EF
#define ETHERNET_FABRIC_SOURCE_IP		0x0A000702
#define ETHERNET_FABRIC_SUBNET_MASK		0xFFFFFF00
#define ETHERNET_GATEWAY_ARP_ADDRESS	0xFF
#define ETHERNET_FABRIC_SUBNET			0x0A000700

#define BOARD_REG				0x1
#define DSP_REG					0x2

#define ARP_RESPONSE			0x2
#define ARP_REQUEST				0x1

#define LINK_UP					0x1
#define LINK_DOWN				0x0

#define NEEDS_RESET				0x1
#define RESET_DONE				0x0
#define LINK_DOWN_RESET_DONE	0x2

#define ARP_REQUESTS_ENABLE		0x1
#define ARP_REQUESTS_DISABLE	0x0

#define ARP_REQUEST_UPDATE		0x1
#define ARP_REQUEST_DONT_UPDATE	0x0

#define MB_ONE_WIRE_PORT		0x0
#define MEZ_0_ONE_WIRE_PORT		0x1
#define MEZ_1_ONE_WIRE_PORT		0x2
#define MEZ_2_ONE_WIRE_PORT		0x3
#define MEZ_3_ONE_WIRE_PORT		0x4

#define REBOOT_REQUESTED		0x1
#define NO_REBOOT				0x2

// COMMAND TYPES
#define HIGHEST_DEFINED_COMMAND	    0x0049//0x0033

#define WRITE_REG					0x0001
#define READ_REG					0x0003
#define WRITE_WISHBONE				0x0005
#define READ_WISHBONE				0x0007
#define WRITE_I2C					0x0009
#define READ_I2C					0x000B
#define SDRAM_RECONFIGURE			0x000D
#define READ_FLASH_WORDS			0x000F
#define PROGRAM_FLASH_WORDS			0x0011
#define ERASE_FLASH_BLOCK			0x0013
#define READ_SPI_PAGE				0x0015
#define PROGRAM_SPI_PAGE			0x0017
#define ERASE_SPI_SECTOR			0x0019
#define ONE_WIRE_READ_ROM_CMD		0x001B
#define ONE_WIRE_DS2433_WRITE_MEM	0x001D
#define ONE_WIRE_DS2433_READ_MEM	0x001F
#define DEBUG_CONFIGURE_ETHERNET	0x0021
#define DEBUG_ADD_ARP_CACHE_ENTRY	0x0023
#define GET_EMBEDDED_SOFTWARE_VERS	0x0025
#define PMBUS_READ_I2C				0x0027
#define SDRAM_PROGRAM				0x0029
#define CONFIGURE_MULTICAST			0x002B
#define DEBUG_LOOPBACK_TEST			0x002D
#define QSFP_RESET_AND_PROG			0x002F
#define HMC_READ_I2C				0x0031
//#define SPARE1                      0x0033
//#define SPARE2                      0x0035
//#define SPARE3                      0x0037
//#define SPARE4                      0x0039
//#define SPARE5                      0x0041


// ETHERNET TYPE CODES
#define ETHERNET_TYPE_IPV4   	0x800
#define ETHERNET_TYPE_ARP    	0x806
#define ETHERNET_TYPE_LLDP      0x88cc

// IP PROTOCOL CODES
#define IP_PROTOCOL_UDP     	0x11
#define IP_PROTOCOL_ICMP		0x01
#define IP_PROTOCOL_IGMP		0x02
#define IP_PING_TOS				0x0

// IGMP DEFINES
#define IGMP_MAC_ADDRESS_HIGH		0x0100
#define IGMP_MAC_ADDRESS_MID		0x5E00
#define IGMP_IP_MASK				0x7FFFFF
#define IGMP_ALL_ROUTERS_IP_ADDRESS	0xE0000002 	// 224.0.0.2
#define IGMP_ALL_SYSTEMS_IP_ADDRESS	0xE0000001	// 224.0.0.1
#define IGMP_QUERY					0x11
#define IGMP_MEMBERSHIP_REPORT		0x16
#define IGMP_LEAVE_REPORT			0x17

#define IGMP_STATE_NOT_JOINED		0x1
#define IGMP_STATE_JOINED_GROUP		0x2
#define IGMP_STATE_LEAVING			0x3

#define IGMP_SEND_MESSAGE			0x1
#define IGMP_DONE_SENDING_MESSAGE	0x2

// DHCP OPTIONS AND CODES
#define DHCP_MAGIC_COOKIE		0x63825363

#define DHCP_SERVER_UDP_PORT	67
#define DHCP_CLIENT_UDP_PORT	68

#define DHCP_MESSAGE_OPTION		53

#define DHCP_MESSAGE_DISCOVER	1
#define DHCP_MESSAGE_OFFER		2
#define DHCP_MESSAGE_REQUEST	3
#define DHCP_MESSAGE_DECLINE	4
#define DHCP_MESSAGE_ACK		5
#define DHCP_MESSAGE_NAK		6
#define DHCP_MESSAGE_RELEASE	7
#define DHCP_MESSAGE_INFORM		8

#define DHCP_PARAMETER_REQUEST_OPTION	55

#define DHCP_PARAMETER_ROUTER			3

//dhcp option macros
#define DHCP_ROUTER_OPTION				  3
#define DHCP_HOST_NAME_OPTION			  12
#define DHCP_SERVER_OPTION				  54
#define DHCP_REQUESTED_IP_OPTION		50
#define DHCP_VENDOR_CLASS_ID_OPTION 60

#define DHCP_END_OPTION			        255
#define DHCP_PAD_OPTION			        0

#define DHCP_STATE_IDLE		0
#define DHCP_STATE_DISCOVER	1
#define DHCP_STATE_REQUEST	2
#define DHCP_STATE_COMPLETE	3

#define DHCP_RETRY_ENABLED	0x1
#define DHCP_RETRY_DISABLED	0x0

#define LLDP_RETRY_ENABLED      0X1
#define LLDP_RETRY_DISABLED     0X0

// TIMER DEFINES
#define DHCP_RETRY_TIMER_ID		0x0
#define DHCP_TIMER_RESET_VALUE	15625000 // (156.25x10^6 x 0.1) 0.1 second

// USB PHY DEFINES
#define USB_I2C_CONTROL			0x100

// LED DEFINES
#define QSFP_STM_I2C_SLAVE_ADDRESS			0x0C
#define QSFP_STM_I2C_BOOTLOADER_SLAVE_ADDRESS	0x08
#define QSFP_LED_TX_REG_ADDRESS				0x02
#define QSFP_LED_RX_REG_ADDRESS				0x03
#define QSFP_MODULE_0_PRESENT_REG_ADDRESS	0x0F
#define QSFP_MODULE_1_PRESENT_REG_ADDRESS	0x1E
#define QSFP_MODULE_2_PRESENT_REG_ADDRESS	0x2D
#define QSFP_MODULE_3_PRESENT_REG_ADDRESS	0x3C

#define QSFP_LEAVE_BOOTLOADER_MODE			0x77

#define QSFP_MEZZANINE_RESET	0x100

#define QSFP0_RESET	0x2
#define QSFP1_RESET	0x4
#define QSFP2_RESET	0x8
#define QSFP3_RESET	0x10

#define LED_OFF 		0x0
#define LED_ON			0x1
#define LED_FLASHING	0x2

#define QSFP_MEZZANINE_PRESENT		0x1
#define QSFP_MEZZANINE_NOT_PRESENT	0x0

#define UPDATE_QSFP_STATUS			0x1
#define DO_NOT_UPDATE_QSFP_STATUS	0x0

#define QSFP_UPDATING_TX_LEDS			0x1
#define QSFP_UPDATING_RX_LEDS			0x2
#define QSFP_UPDATING_MOD_PRSNT_0_WRITE	0x3
#define QSFP_UPDATING_MOD_PRSNT_0_READ	0x4
#define QSFP_UPDATING_MOD_PRSNT_1_WRITE	0x5
#define QSFP_UPDATING_MOD_PRSNT_1_READ	0x6
#define QSFP_UPDATING_MOD_PRSNT_2_WRITE	0x7
#define QSFP_UPDATING_MOD_PRSNT_2_READ	0x8
#define QSFP_UPDATING_MOD_PRSNT_3_WRITE	0x9
#define QSFP_UPDATING_MOD_PRSNT_3_READ	0xA

#define QSFP_I2C_MICROBLAZE_ENABLE		0x1
#define QSFP_I2C_MICROBLAZE_DISABLE		0x0

#define QSFP_I2C_ACCESS_TIMEOUT			500000

#define QSFP_STATE_RESET								0x1
#define QSFP_STATE_BOOTLOADER_VERSION_WRITE_MODE		0x2
#define QSFP_STATE_BOOTLOADER_VERSION_READ_MODE			0x3
#define QSFP_STATE_INITIAL_BOOTLOADER_MODE				0x4
#define QSFP_STATE_STARTING_APPLICATION_MODE			0x5
#define QSFP_STATE_APPLICATION_MODE						0x6
#define QSFP_STATE_BOOTLOADER_PROGRAMMING_MODE			0x7

#define QSFP_BOOTLOADER_READ_OPCODE						0x03
#define QSFP_BOOTLOADER_VERSION_ADDRESS					0x08007000

typedef struct sEthernetHeader {
	u16 uDestMacHigh;
	u16 uDestMacMid;

	u16 uDestMacLow;
	u16 uSourceMacHigh;

	u16 uSourceMacMid;
	u16 uSourceMacLow;

	u16 uEthernetType;
} sEthernetHeaderT;

typedef struct sArpPacket {
	u16 uHardwareType;
	u16	uProtocolType;

	u8  uProtocolLength;
	u8  uHardwareLength;
	u16 uOperation;

	u16 uSenderMacHigh;
	u16 uSenderMacMid;

	u16 uSenderMacLow;
	u16 uSenderIpHigh;

	u16 uSenderIPLow;
	u16 uTargetMacHigh;

	u16 uTargetMacMid;
	u16 uTargetMacLow;

	u16 uTargetIPHigh;
	u16 uTargetIPLow;

	u16 uPadding[11];
} sArpPacketT;

typedef struct sIPV4Header {
	u8   uTypeOfService;
	u8   uVersion;
	u16  uTotalLength;

	u16  uIdentification;
	u16  uFlagsFragment;

	u8   uProtocol;
	u8   uTimeToLive;
	u16  uChecksum;

	u16  uSourceIPHigh;
	u16  uSourceIPLow;
	u16  uDestinationIPHigh;
	u16  uDestinationIPLow;
} sIPV4HeaderT;

typedef struct sICMPHeader {
	u8 	uCode;
	u8 	uType;
	u16 uChecksum;
	u16 uIdentifier;
	u16 uSequenceNumber;
} sICMPHeaderT;

typedef struct sIGMPHeader {
	u8 uMaximumResponseTime;
	u8 uType;
	u16 uChecksum;

	u16 uGroupAddressHigh;
	u16 uGroupAddressLow;

	u16 uPadding[11];
} sIGMPHeaderT;

typedef struct sUDPHeader {
	u16  uSourcePort;
	u16  uDestinationPort;

	u16  uTotalLength;
	u16  uChecksum;
} sUDPHeaderT;

typedef struct sDHCPHeader {
	u8 	uHardwareType;
	u8 	uOpCode;
	u8 	uHops;
	u8 	uHardwareAddressLength;

	u16 uXidHigh;
	u16	uXidLow;

	u16	uSeconds;
	u16	uFlags;

	u16	uClientIPAddressHigh;
	u16 uClientIPAddressLow;

	u16	uYourIPAddressHigh;
	u16	uYourIPAddressLow;

	u16	uServerIPAddressHigh;
	u16	uServerIPAddressLow;

	u16	uGatewayIPAddressHigh;
	u16	uGatewayIPAddressLow;

	u16	uClientHardwareAddress[8];

	u16	uBootPLegacy[96];

	u16 uMagicCookieHigh;
	u16 uMagicCookieLow;
} sDHCPHeaderT;

typedef struct sCommandHeader {
    u16  uCommandType;
    u16  uSequenceNumber;
} sCommandHeaderT;

typedef struct sGetEmbeddedSoftwareVersionReq {
	sCommandHeaderT Header;
} sGetEmbeddedSoftwareVersionReqT;

typedef struct sGetEmbeddedSoftwareVersionResp {
	sCommandHeaderT Header;
	u16				uVersionMajor;
	u16				uVersionMinor;
	u16				uVersionPatch;
	u16 			uQSFPBootloaderVersionMajor;
	u16				uQSFPBootloaderVersionMinor;
	u16				uPadding[4];
} sGetEmbeddedSoftwareVersionRespT;

typedef struct sWriteRegReq {
	sCommandHeaderT Header;
    u16  			uBoardReg;
    u16				uRegAddress;
    u16				uRegDataHigh;
    u16				uRegDataLow;
} sWriteRegReqT;

typedef struct sWriteRegResp {
	sCommandHeaderT Header;
    u16  			uBoardReg;
    u16				uRegAddress;
    u16				uRegDataHigh;
    u16				uRegDataLow;
    u16				uPadding[5];
} sWriteRegRespT;

typedef struct sReadRegReq {
	sCommandHeaderT Header;
    u16  			uBoardReg;
    u16				uRegAddress;
} sReadRegReqT;

typedef struct sReadRegResp {
	sCommandHeaderT	Header;
    u16  			uBoardReg;
    u16				uRegAddress;
    u16				uRegDataHigh;
    u16				uRegDataLow;
    u16				uPadding[5];
} sReadRegRespT;

typedef struct sWriteWishboneReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uWriteDataHigh;
    u16				uWriteDataLow;
} sWriteWishboneReqT;

typedef struct sWriteWishboneResp {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uWriteDataHigh;
    u16				uWriteDataLow;
    u16				uPadding[5];
} sWriteWishboneRespT;

typedef struct sReadWishboneReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
} sReadWishboneReqT;

typedef struct sReadWishboneResp {
	sCommandHeaderT	Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uReadDataHigh;
    u16				uReadDataLow;
    u16				uPadding[5];
} sReadWishboneRespT;

typedef struct sWriteI2CReq {
	sCommandHeaderT Header;
    u16				uId;
    u16				uSlaveAddress;
    u16				uNumBytes;
    u16				uWriteBytes[32];
} sWriteI2CReqT;

typedef struct sWriteI2CResp {
	sCommandHeaderT Header;
    u16				uId;
    u16				uSlaveAddress;
    u16				uNumBytes;
    u16				uWriteBytes[32];
    u16				uWriteSuccess;
    u16				uPadding[1];
} sWriteI2CRespT;

typedef struct sReadI2CReq {
	sCommandHeaderT Header;
    u16				uId;
    u16				uSlaveAddress;
    u16				uNumBytes;
} sReadI2CReqT;

typedef struct sReadI2CResp {
	sCommandHeaderT Header;
    u16				uId;
    u16				uSlaveAddress;
    u16				uNumBytes;
    u16				uReadBytes[32];
    u16				uReadSuccess;
    u16				uPadding[1];
} sReadI2CRespT;

typedef struct sSdramReconfigureReq {
	sCommandHeaderT Header;
	u16				uOutputMode;
	u16 			uClearSdram;
	u16				uFinishedWritingToSdram;
	u16				uAboutToBootFromSdram;
	u16				uDoReboot;
	u16				uResetSdramReadAddress;
	u16				uClearEthernetStatistics;
	u16				uEnableDebugSdramReadMode;
	u16				uDoSdramAsyncRead;
	u16				uDoContinuityTest;
	u16				uContinuityTestOutputLow;
	u16				uContinuityTestOutputHigh;
} sSdramReconfigureReqT;

typedef struct sSdramReconfigureResp {
	sCommandHeaderT Header;
	u16				uOutputMode;
	u16 			uClearSdram;
	u16				uFinishedWritingToSdram;
	u16				uAboutToBootFromSdram;
	u16				uDoReboot;
	u16				uResetSdramReadAddress;
	u16				uClearEthernetStatistics;
	u16				uEnableDebugSdramReadMode;
	u16				uDoSdramAsyncRead;
	u16				uNumEthernetFrames;
	u16				uNumEthernetBadFrames;
	u16				uNumEthernetOverloadFrames;
	u16				uSdramAsyncReadDataHigh;
	u16				uSdramAsyncReadDataLow;
	u16				uDoContinuityTest;
	u16				uContinuityTestReadLow;
	u16				uContinuityTestReadHigh;
} sSdramReconfigureRespT;

typedef struct sReadFlashWordsReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumWords;
} sReadFlashWordsReqT;

typedef struct sReadFlashWordsResp {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumWords;
    u16				uReadWords[384];
    u16				uPadding[2];
} sReadFlashWordsRespT;

typedef struct sProgramFlashWordsReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16 			uTotalNumWords;
    u16				uNumWords;
    u16				uDoBufferedProgramming;
    u16				uStartProgram;
    u16				uFinishProgram;
    u16				uWriteWords[256];
} sProgramFlashWordsReqT;

typedef struct sProgramFlashWordsResp {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16 			uTotalNumWords;
    u16				uNumWords;
    u16				uDoBufferedProgramming;
    u16				uStartProgram;
    u16				uFinishProgram;
    u16				uProgramSuccess;
    u16				uPadding[1];
} sProgramFlashWordsRespT;

typedef struct sEraseFlashBlockReq {
	sCommandHeaderT Header;
    u16				uBlockAddressHigh;
    u16				uBlockAddressLow;
} sEraseFlashBlockReqT;

typedef struct sEraseFlashBlockResp {
	sCommandHeaderT Header;
    u16				uBlockAddressHigh;
    u16				uBlockAddressLow;
    u16				uEraseSuccess;
    u16				uPadding[6];
} sEraseFlashBlockRespT;

typedef struct sReadSpiPageReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumBytes;
} sReadSpiPageReqT;

typedef struct sReadSpiPageResp {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumBytes;
    u16				uReadBytes[264];
    u16				uReadSpiPageSuccess;
    u16				uPadding[1];
} sReadSpiPageRespT;

typedef struct sProgramSpiPageReq {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumBytes;
    u16				uWriteBytes[264];
} sProgramSpiPageReqT;

typedef struct sProgramSpiPageResp {
	sCommandHeaderT Header;
    u16				uAddressHigh;
    u16				uAddressLow;
    u16				uNumBytes;
    u16				uVerifyBytes[264];
    u16				uProgramSpiPageSuccess;
    u16				uPadding[1];
} sProgramSpiPageRespT;

typedef struct sEraseSpiSectorReq {
	sCommandHeaderT Header;
    u16				uSectorAddressHigh;
    u16				uSectorAddressLow;
} sEraseSpiSectorReqT;

typedef struct sEraseSpiSectorResp {
	sCommandHeaderT Header;
    u16				uSectorAddressHigh;
    u16				uSectorAddressLow;
    u16				uEraseSuccess;
    u16				uPadding[6];
} sEraseSpiSectorRespT;

typedef struct sOneWireReadROMReq {
	sCommandHeaderT Header;
    u16				uOneWirePort;
} sOneWireReadROMReqT;

typedef struct sOneWireReadROMResp {
	sCommandHeaderT Header;
    u16				uOneWirePort;
    u16				uRom[8];
    u16				uReadSuccess;
    u16				uPadding[3];
} sOneWireReadROMRespT;

typedef struct sOneWireDS2433WriteMemReq {
	sCommandHeaderT Header;
	u16				uDeviceRom[8];
	u16				uSkipRomAddress;
	u16				uWriteBytes[32];
	u16				uNumBytes;
	u16				uTA1;
	u16				uTA2;
    u16				uOneWirePort;
} sOneWireDS2433WriteMemReqT;

typedef struct sOneWireDS2433WriteMemResp {
	sCommandHeaderT Header;
	u16				uDeviceRom[8];
	u16				uSkipRomAddress;
	u16				uWriteBytes[32];
	u16				uNumBytes;
	u16				uTA1;
	u16				uTA2;
    u16				uOneWirePort;
    u16				uWriteSuccess;
    u16				uPadding[3];
} sOneWireDS2433WriteMemRespT;

typedef struct sOneWireDS2433ReadMemReq {
	sCommandHeaderT Header;
	u16				uDeviceRom[8];
	u16				uSkipRomAddress;
	u16				uNumBytes;
	u16				uTA1;
	u16				uTA2;
    u16				uOneWirePort;
} sOneWireDS2433ReadMemReqT;

typedef struct sOneWireDS2433ReadMemResp {
	sCommandHeaderT Header;
	u16				uDeviceRom[8];
	u16				uSkipRomAddress;
	u16				uReadBytes[32];
	u16				uNumBytes;
	u16				uTA1;
	u16				uTA2;
    u16				uOneWirePort;
    u16				uReadSuccess;
    u16				uPadding[3];
} sOneWireDS2433ReadMemRespT;

typedef struct sDebugConfigureEthernetReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uFabricMacHigh;
	u16				uFabricMacMid;
	u16				uFabricMacLow;
	u16				uFabricPortAddress;
	u16				uGatewayIPAddressHigh;
	u16				uGatewayIPAddressLow;
	u16				uFabricIPAddressHigh;
	u16				uFabricIPAddressLow;
	u16				uFabricMultiCastIPAddressHigh;
	u16				uFabricMultiCastIPAddressLow;
	u16				uFabricMultiCastIPAddressMaskHigh;
	u16				uFabricMultiCastIPAddressMaskLow;
	u16				uEnableFabricInterface;
} sDebugConfigureEthernetReqT;

typedef struct sDebugConfigureEthernetResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uFabricMacHigh;
	u16				uFabricMacMid;
	u16				uFabricMacLow;
	u16				uFabricPortAddress;
	u16				uGatewayIPAddressHigh;
	u16				uGatewayIPAddressLow;
	u16				uFabricIPAddressHigh;
	u16				uFabricIPAddressLow;
	u16				uFabricMultiCastIPAddressHigh;
	u16				uFabricMultiCastIPAddressLow;
	u16				uFabricMultiCastIPAddressMaskHigh;
	u16				uFabricMultiCastIPAddressMaskLow;
	u16				uEnableFabricInterface;
	u16				uPadding[3];
} sDebugConfigureEthernetRespT;

typedef struct sDebugAddARPCacheEntryReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uIPAddressLower8Bits;
	u16				uMacHigh;
	u16				uMacMid;
	u16				uMacLow;
} sDebugAddARPCacheEntryReqT;

typedef struct sDebugAddARPCacheEntryResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uIPAddressLower8Bits;
	u16				uMacHigh;
	u16				uMacMid;
	u16				uMacLow;
	u16				uPadding[4];
} sDebugAddARPCacheEntryRespT;

typedef struct sPMBusReadI2CBytesReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uSlaveAddress;
	u16				uCommandCode;
	u16				uReadBytes[32];
	u16				uNumBytes;
} sPMBusReadI2CBytesReqT;

typedef struct sPMBusReadI2CBytesResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uSlaveAddress;
	u16				uCommandCode;
	u16				uReadBytes[32];
	u16				uNumBytes;
    u16				uReadSuccess;
} sPMBusReadI2CBytesRespT;

typedef struct sConfigureMulticastReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uFabricMultiCastIPAddressHigh;
	u16				uFabricMultiCastIPAddressLow;
	u16				uFabricMultiCastIPAddressMaskHigh;
	u16				uFabricMultiCastIPAddressMaskLow;
} sConfigureMulticastReqT;

typedef struct sConfigureMulticastResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uFabricMultiCastIPAddressHigh;
	u16				uFabricMultiCastIPAddressLow;
	u16				uFabricMultiCastIPAddressMaskHigh;
	u16				uFabricMultiCastIPAddressMaskLow;
	u16				uPadding[4];
} sConfigureMulticastRespT;

typedef struct sDebugLoopbackTestReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uTestData[256];
} sDebugLoopbackTestReqT;

typedef struct sDebugLoopbackTestResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uTestData[256];
	u16				uValid;
	u16				uPadding[3];
} sDebugLoopbackTestRespT;

typedef struct sQSFPResetAndProgramReq {
	sCommandHeaderT Header;
    u16  			uReset;
    u16				uProgram;
} sQSFPResetAndProgramReqT;

typedef struct sQSFPResetAndProgramResp {
	sCommandHeaderT Header;
    u16  			uReset;
    u16				uProgram;
	u16				uPadding[7];
} sQSFPResetAndProgramRespT;

typedef struct sHMCReadI2CBytesReq {
	sCommandHeaderT Header;
	u16				uId;
	u16				uSlaveAddress;
	u16				uReadAddress[4];
} sHMCReadI2CBytesReqT;

typedef struct sHMCReadI2CBytesResp {
	sCommandHeaderT Header;
	u16				uId;
	u16				uSlaveAddress;
	u16				uReadAddress[4];
	u16				uReadBytes[4];
    u16				uReadSuccess;
	u16				uPadding[2];
} sHMCReadI2CBytesRespT;

// I2C BUS DEFINES
#define	MB_I2C_BUS_ID				0x0
#define MEZZANINE_0_I2C_BUS_ID		0x1
#define MEZZANINE_1_I2C_BUS_ID		0x2
#define MEZZANINE_2_I2C_BUS_ID		0x3
#define MEZZANINE_3_I2C_BUS_ID		0x4

#endif
