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

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

// GLOBAL VARIABLES

#define NUM_ETHERNET_INTERFACES     5u//AI: Single 40GbE Core 0x5
//#define DO_1GBE_LOOPBACK_TEST


//uncomment relevant print output level
//macro logic in print.h
//#define ERROR_PRINT
//#define WARN_PRINT
//#define INFO_PRINT
#define DEBUG_PRINT
/* trace level will lower performance significantly */
//#define TRACE_PRINT

//#define DO_40GBE_LOOPBACK_TEST

#define NUM_REGISTERS       32

volatile u32 uWriteBoardShadowRegs[NUM_REGISTERS];

// Single transmit buffer
#define TX_BUFFER_MAX 256
volatile u32 uTransmitBuffer[TX_BUFFER_MAX];

// Single receive buffer
//#define RX_BUFFER_MAX 512
#define RX_BUFFER_MAX 2254  /* Allow for jumbo packets i.e. MTU 9000.
                               Assuming MTU means maximum L3 packet size in bytes,
                               we require 9000 plus extra 14 bytes for eth header
                               (preamble and FCS handled in FW).
                               2254 x 32bit words = 9016 bytes */
//volatile u32 uReceiveBuffer[NUM_ETHERNET_INTERFACES][RX_BUFFER_MAX]; // GT 30/03/2017 NEEDS TO MATCH ACTUAL SIZE IN FIRMWARE
volatile u32 uReceiveBuffer[RX_BUFFER_MAX]; // GT 30/03/2017 NEEDS TO MATCH ACTUAL SIZE IN FIRMWARE

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
/* volatile u32 uEthernetFabricIPAddress[NUM_ETHERNET_INTERFACES]; */
/* volatile u32 uEthernetFabricSubnetMask[NUM_ETHERNET_INTERFACES]; */
/* volatile u16 uEthernetFabricPortAddress[NUM_ETHERNET_INTERFACES]; */
volatile u32 uEthernetFabricMultiCastIPAddress[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetFabricMultiCastIPAddressMask[NUM_ETHERNET_INTERFACES];
volatile u16 uIPIdentification[NUM_ETHERNET_INTERFACES];
//volatile u32 uEthernetSubnet[NUM_ETHERNET_INTERFACES];
volatile u32 uEthernetGatewayIPAddress[NUM_ETHERNET_INTERFACES];

/* volatile u8 uEthernetLinkUp[NUM_ETHERNET_INTERFACES]; */
/* volatile u8 uEthernetNeedsReset[NUM_ETHERNET_INTERFACES]; */

volatile u32 uLLDPTimerCounter;
volatile u8 uLLDPRetryTimer[NUM_ETHERNET_INTERFACES];

//volatile u32 uDHCPTimerCounter;
volatile u8 uDHCPState[NUM_ETHERNET_INTERFACES];
/* volatile u8 uDHCPRetryTimer[NUM_ETHERNET_INTERFACES]; */

//volatile u8 uIGMPTimerCounter;
//volatile u8 uIGMPState[NUM_ETHERNET_INTERFACES];
//volatile u8 uIGMPSendMessage[NUM_ETHERNET_INTERFACES];
//volatile u8 uCurrentIGMPMessage[NUM_ETHERNET_INTERFACES];

volatile u32 uPrintStatsCounter;

volatile u8 uDoReboot;

volatile u8 uQSFPMezzanineLocation;
volatile u8 uQSFPMezzaninePresent;
volatile u32 uQSFPCtrlReg;
//volatile u8 uQSFPUpdateStatusEnable;
/* volatile u8 uQSFPUpdateState; */
volatile u8 uQSFPI2CMicroblazeAccess;
/* volatile u32 uQSFPStateCounter; */
/* volatile u8 uQSFPState; */

volatile u16 GlobalDHCPMonitorTimeout;   /* TODO: hide/wrap this global var in a module*/

volatile u16 uQSFPBootloaderVersionMajor;
volatile u16 uQSFPBootloaderVersionMinor;

volatile u32 uPreviousAsyncSdramRead;
volatile u16 uPreviousSequenceNumber;

//volatile u8 uADC32RF45X2MezzanineLocation;
//volatile u8 uADC32RF45X2MezzaninePresent;
volatile u8 uADC32RF45X2UpdateStatusEnable;

volatile u16 uADC32RF45X2BootloaderVersionMajor;
volatile u16 uADC32RF45X2BootloaderVersionMinor;

/* <major>.<minor>.<patch> */
#define EMBEDDED_SOFTWARE_VERSION_MAJOR   3
#define EMBEDDED_SOFTWARE_VERSION_MINOR   21      /* year */
#define EMBEDDED_SOFTWARE_VERSION_PATCH   127     /* day of the year */

// WISHBONE SLAVE ADDRESSES
#define BOARD_REGISTER_ADDR     0x00000000
#define FLASH_SDRAM_SPI_ICAPE_ADDR  0x00010000
#define ONE_WIRE_ADDR       0x00018000
#define I2C_0_ADDR          0x00020000
#define I2C_1_ADDR          0x00028000
#define I2C_2_ADDR          0x00030000
#define I2C_3_ADDR          0x00038000
#define I2C_4_ADDR          0x00040000

#ifdef WISHBONE_LEGACY_MAP
/* original/legacy wishbone slave map */
#define ONE_GBE_MAC_ADDR        0x00048000
#define FORTY_GBE_MAC_0_ADDR    0x00050000
#define FORTY_GBE_MAC_1_ADDR    0x00058000
#define FORTY_GBE_MAC_2_ADDR    0x00060000
#define FORTY_GBE_MAC_3_ADDR    0x00068000
#define DSP_REGISTER_ADDR       0x00070000
#else
/* adjusted wishbone slave address map - provision for "jumbo" ethernet support */
/* These offsets are calculated as follows in firmware (each core aloocated 0x16000 of address space):
 * core index x 0x16000 + dsp-offset of 0x84000
 * Note : 1gbe = core number 5 with index 4
 */
#define FORTY_GBE_MAC_0_ADDR    0x84000     /* 0x84000 + (0 x 0x16000) */
#define FORTY_GBE_MAC_1_ADDR    0x9A000     /* 0x84000 + (1 x 0x16000) */
#define FORTY_GBE_MAC_2_ADDR    0xB0000     /* 0x84000 + (2 x 0x16000) */
#define FORTY_GBE_MAC_3_ADDR    0xC6000     /* 0x84000 + (3 x 0x16000) */
#define ONE_GBE_MAC_ADDR        0xDC000     /* 0x84000 + (4 x 0x16000) */
#define DSP_REGISTER_ADDR       0x84000
#endif

/*#define DUAL_PORT_RAM_ADDR      0x00000
#define BOARD_REGISTER_ADDR     0x08000
#define ONE_GBE_MAC_ADDR      0x10000
#define FORTY_GBE_MAC_0_ADDR    0x18000
#define FLASH_SDRAM_SPI_ICAPE_ADDR  0x20000
#define ONE_WIRE_ADDR       0x28000
#define I2C_0_ADDR          0x30000
#define I2C_1_ADDR          0x38000
#define I2C_2_ADDR          0x40000
#define DSP_REGISTER_ADDR     0xF8000*/

// BOARD REGISTER OFFSET
// READ REGISTERS
#define C_RD_VERSION_ADDR            0x0
#define C_RD_BRD_CTL_STAT_0_ADDR     0x4
#define C_RD_LOOPBACK_ADDR           0x8
#define C_RD_ETH_IF_LINK_UP_ADDR     0xC
#define C_RD_MEZZANINE_STAT_0_ADDR   0x10
#define C_RD_USB_STAT_ADDR           0x14
#define C_RD_FPGA_DNA_LOW_ADDR       0x1C
#define C_RD_FPGA_DNA_HIGH_ADDR      0x20
#define C_RD_XADC_STATUS_ADDR        0x24
#define C_RD_XADC_LATCHED_ADDR       0x28
#define C_RD_UBLAZE_ALIVE_ADDR       0x2C
#define C_RD_MEZZANINE_STAT_1_ADDR   0x30

// WRITE REGISTERS
#define C_WR_BRD_CTL_STAT_0_ADDR        0x4
#define C_WR_LOOPBACK_ADDR              0x8
#define C_WR_ETH_IF_CTL_ADDR            0xC
#define C_WR_MEZZANINE_CTL_ADDR         0x10
#define C_WR_FRONT_PANEL_STAT_LED_ADDR  0x14
#define C_WR_BRD_CTL_STAT_1_ADDR        0x18
#define C_WR_UBLAZE_ALIVE_ADDR          0x2C

// DSP REGISTER ADDRESSES

#define ETHERNET_FABRIC_PORT_ADDRESS  0x7148
#define ETHERNET_CONTROL_PORT_ADDRESS 0x7778
#define ETHERNET_FABRIC_SOURCE_MAC_HIGH 0x000B
#define ETHERNET_FABRIC_SOURCE_MAC_MID  0x0E0F
#define ETHERNET_FABRIC_SOURCE_MAC_LOW  0x00EF
#define ETHERNET_FABRIC_SOURCE_IP   0x0A000702
#define ETHERNET_FABRIC_SUBNET_MASK   0xFFFFFF00
#define ETHERNET_GATEWAY_ARP_ADDRESS  0xFF
#define ETHERNET_FABRIC_SUBNET      0x0A000700

#define BOARD_REG       0x1
#define DSP_REG         0x2

#define ARP_RESPONSE      0x2
#define ARP_REQUEST       0x1

#define LINK_UP         0x1
#define LINK_DOWN       0x0

#define NEEDS_RESET       0x1
#define RESET_DONE        0x0
#define LINK_DOWN_RESET_DONE  0x2

#define ARP_REQUESTS_ENABLE   0x1
#define ARP_REQUESTS_DISABLE  0x0

#define ARP_REQUEST_UPDATE    0x1
#define ARP_REQUEST_DONT_UPDATE 0x0

#define MB_ONE_WIRE_PORT    0x0
#define MEZ_0_ONE_WIRE_PORT   0x1
#define MEZ_1_ONE_WIRE_PORT   0x2
#define MEZ_2_ONE_WIRE_PORT   0x3
#define MEZ_3_ONE_WIRE_PORT   0x4

#define REBOOT_REQUESTED    0x1
#define NO_REBOOT       0x2

// COMMAND TYPES
#define WRITE_REG         0x0001
#define READ_REG          0x0003
#define WRITE_WISHBONE        0x0005
#define READ_WISHBONE       0x0007
#define WRITE_I2C         0x0009
#define READ_I2C          0x000B
#define SDRAM_RECONFIGURE     0x000D
#define READ_FLASH_WORDS      0x000F
#define PROGRAM_FLASH_WORDS     0x0011
#define ERASE_FLASH_BLOCK     0x0013
#define READ_SPI_PAGE       0x0015
#define PROGRAM_SPI_PAGE      0x0017
#define ERASE_SPI_SECTOR      0x0019
#define ONE_WIRE_READ_ROM_CMD   0x001B
#define ONE_WIRE_DS2433_WRITE_MEM 0x001D
#define ONE_WIRE_DS2433_READ_MEM  0x001F
#define DEBUG_CONFIGURE_ETHERNET  0x0021
#define DEBUG_ADD_ARP_CACHE_ENTRY 0x0023
#define GET_EMBEDDED_SOFTWARE_VERS  0x0025
#define PMBUS_READ_I2C        0x0027
#define SDRAM_PROGRAM       0x0029
#define CONFIGURE_MULTICAST     0x002B
#define DEBUG_LOOPBACK_TEST     0x002D
#define QSFP_RESET_AND_PROG     0x002F
#define HMC_READ_I2C        0x0031
#define HMC_WRITE_I2C               0x0033
//#define SPARE2                      0x0035
//#define SPARE3                      0x0037
#define ADC_MEZZANINE_RESET_AND_PROG	  0x0039
//#define SPARE5                      0x0041

/* the following opcodes (commented out) defined in custom_constants.h
 * #define GET_SENSOR_DATA       0x0043
 * #define SET_FAN_SPEED         0x0045
 * #define BIG_READ_WISHBONE     0x0047
 * #define BIG_WRITE_WISHBONE    0x0049
 */

#define SDRAM_PROGRAM_OVER_WISHBONE 0x0051
#define SET_DHCP_TUNING_DEBUG       0x0053
#define GET_DHCP_TUNING_DEBUG       0x0055
#define GET_CURRENT_LOGS            0x0057
#define GET_VOLTAGE_LOGS            0x0059
#define GET_FANCONTROLLER_LOGS      0x005B
#define CLEAR_FANCONTROLLER_LOGS    0x005D
#define DHCP_RESET_STATE_MACHINE    0x005F
#define MULTICAST_LEAVE_GROUP       0x0061
#define GET_DHCP_MONITOR_TIMEOUT    0x0063
#define GET_MICROBLAZE_UPTIME       0x0065
#define FPGA_FANCONTROLLER_UPDATE   0x0067
#define GET_FPGA_FANCONTROLLER_LUT  0x0069
#define HIGHEST_DEFINED_COMMAND     0x0069


// ETHERNET TYPE CODES
#define ETHERNET_TYPE_IPV4    0x800
#define ETHERNET_TYPE_ARP     0x806
#define ETHERNET_TYPE_LLDP      0x88cc

// IP PROTOCOL CODES
#define IP_PROTOCOL_UDP       0x11
#define IP_PROTOCOL_ICMP    0x01
#define IP_PROTOCOL_IGMP    0x02
#define IP_PING_TOS       0x0

// IGMP DEFINES
#define IGMP_MAC_ADDRESS_HIGH   0x0100
#define IGMP_MAC_ADDRESS_MID    0x5E00
#define IGMP_IP_MASK        0x7FFFFF
#define IGMP_ALL_ROUTERS_IP_ADDRESS 0xE0000002  // 224.0.0.2
#define IGMP_ALL_SYSTEMS_IP_ADDRESS 0xE0000001  // 224.0.0.1
#define IGMP_QUERY          0x11
#define IGMP_MEMBERSHIP_REPORT    0x16
#define IGMP_LEAVE_REPORT     0x17

#define IGMP_STATE_NOT_JOINED   0x1
#define IGMP_STATE_JOINED_GROUP   0x2
#define IGMP_STATE_LEAVING      0x3

#define IGMP_SEND_MESSAGE     0x1
#define IGMP_DONE_SENDING_MESSAGE 0x2

#define DHCP_STATE_IDLE   0
/* #define DHCP_STATE_DISCOVER 1 */
/* #define DHCP_STATE_REQUEST  2 */
#define DHCP_STATE_COMPLETE 3

//#define DHCP_RETRY_ENABLED  0x1
//#define DHCP_RETRY_DISABLED 0x0

#define LLDP_RETRY_ENABLED      0X1
#define LLDP_RETRY_DISABLED     0X0

// TIMER DEFINES
#define DHCP_RETRY_TIMER_ID   0x0
#define AUX_RETRY_TIMER_ID    0x1

#ifdef REDUCED_CLK_ARCH
#define DHCP_TIMER_RESET_VALUE     3906250 // (39.0625x10^6 x 0.1) 0.1 second
#define AUX_TIMER_RESET_VALUE      39062500 // 1 second
#else
#define DHCP_TIMER_RESET_VALUE  15625000 // (156.25x10^6 x 0.1) 0.1 second
#endif

// USB PHY DEFINES
#define USB_I2C_CONTROL     0x100

// LED DEFINES
#define QSFP_STM_I2C_SLAVE_ADDRESS      0x0C
#define QSFP_STM_I2C_BOOTLOADER_SLAVE_ADDRESS 0x08
#define QSFP_LED_TX_REG_ADDRESS       0x02
#define QSFP_LED_RX_REG_ADDRESS       0x03
#define QSFP_MODULE_0_PRESENT_REG_ADDRESS 0x0F
#define QSFP_MODULE_1_PRESENT_REG_ADDRESS 0x1E
#define QSFP_MODULE_2_PRESENT_REG_ADDRESS 0x2D
#define QSFP_MODULE_3_PRESENT_REG_ADDRESS 0x3C

#define QSFP_LEAVE_BOOTLOADER_MODE      0x77

#define QSFP_MEZZANINE_RESET  0x100

#define QSFP0_RESET 0x2
#define QSFP1_RESET 0x4
#define QSFP2_RESET 0x8
#define QSFP3_RESET 0x10

#define LED_OFF     0x0
#define LED_ON      0x1
#define LED_FLASHING  0x2

#define QSFP_MEZZANINE_PRESENT    0x1
#define QSFP_MEZZANINE_NOT_PRESENT  0x0

#define UPDATE_QSFP_STATUS      0x1
#define DO_NOT_UPDATE_QSFP_STATUS 0x0

#define QSFP_UPDATING_TX_LEDS     0x1
#define QSFP_UPDATING_RX_LEDS     0x2
#define QSFP_UPDATING_MOD_PRSNT_0_WRITE 0x3
#define QSFP_UPDATING_MOD_PRSNT_0_READ  0x4
#define QSFP_UPDATING_MOD_PRSNT_1_WRITE 0x5
#define QSFP_UPDATING_MOD_PRSNT_1_READ  0x6
#define QSFP_UPDATING_MOD_PRSNT_2_WRITE 0x7
#define QSFP_UPDATING_MOD_PRSNT_2_READ  0x8
#define QSFP_UPDATING_MOD_PRSNT_3_WRITE 0x9
#define QSFP_UPDATING_MOD_PRSNT_3_READ  0xA

#define QSFP_I2C_MICROBLAZE_ENABLE    0x1
#define QSFP_I2C_MICROBLAZE_DISABLE   0x0

#define QSFP_I2C_ACCESS_TIMEOUT     500000

#define QSFP_STATE_RESET                0x1
#define QSFP_STATE_BOOTLOADER_VERSION_WRITE_MODE    0x2
#define QSFP_STATE_BOOTLOADER_VERSION_READ_MODE     0x3
#define QSFP_STATE_INITIAL_BOOTLOADER_MODE        0x4
#define QSFP_STATE_STARTING_APPLICATION_MODE      0x5
#define QSFP_STATE_APPLICATION_MODE           0x6
#define QSFP_STATE_BOOTLOADER_PROGRAMMING_MODE      0x7

#define QSFP_BOOTLOADER_READ_OPCODE           0x03
#define QSFP_BOOTLOADER_VERSION_ADDRESS         0x08007000


#define ADC32RF45X2_STM_I2C_BOOTLOADER_SLAVE_ADDRESS  0x08
#define ADC32RF45X2_LEAVE_BOOTLOADER_MODE             0x77

#define ADC32RF45X2_MEZZANINE_PRESENT     0x1
#define ADC32RF45X2_MEZZANINE_NOT_PRESENT 0x0

#define UPDATE_ADC32RF45X2_STATUS         0x1
#define DO_NOT_UPDATE_ADC32RF45X2_STATUS  0x0

#define ADC32RF45X2_BOOTLOADER_READ_OPCODE        0x03
#define ADC32RF45X2_BOOTLOADER_VERSION_ADDRESS    0x0800F000

typedef struct sEthernetHeader {
  u16 uDestMacHigh;
  u16 uDestMacMid;

  u16 uDestMacLow;
  u16 uSourceMacHigh;

  u16 uSourceMacMid;
  u16 uSourceMacLow;

  u16 uEthernetType;
} sEthernetHeaderT;

#if 0
typedef struct sArpPacket {
  u16 uHardwareType;
  u16 uProtocolType;

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
#endif

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

/* added to allow Router Alert Option for IGMP v2 */
typedef struct sIPV4HeaderOptions {
  u16  uOptionsHigh;
  u16  uOptionsLow;
} sIPV4HeaderOptionsT;

#if 0
typedef struct sICMPHeader {
  u8  uCode;
  u8  uType;
  u16 uChecksum;
  u16 uIdentifier;
  u16 uSequenceNumber;
} sICMPHeaderT;
#endif

typedef struct sIGMPHeader {
  u8 uMaximumResponseTime;
  u8 uType;
  u16 uChecksum;

  u16 uGroupAddressHigh;
  u16 uGroupAddressLow;

  u16 uPadding[9];
} sIGMPHeaderT;

typedef struct sUDPHeader {
  u16  uSourcePort;
  u16  uDestinationPort;

  u16  uTotalLength;
  u16  uChecksum;
} sUDPHeaderT;

#if 0
typedef struct sDHCPHeader {
  u8  uHardwareType;
  u8  uOpCode;
  u8  uHops;
  u8  uHardwareAddressLength;

  u16 uXidHigh;
  u16 uXidLow;

  u16 uSeconds;
  u16 uFlags;

  u16 uClientIPAddressHigh;
  u16 uClientIPAddressLow;

  u16 uYourIPAddressHigh;
  u16 uYourIPAddressLow;

  u16 uServerIPAddressHigh;
  u16 uServerIPAddressLow;

  u16 uGatewayIPAddressHigh;
  u16 uGatewayIPAddressLow;

  u16 uClientHardwareAddress[8];

  u16 uBootPLegacy[96];

  u16 uMagicCookieHigh;
  u16 uMagicCookieLow;
} sDHCPHeaderT;
#endif

/* return this for any uStatus fields in the command response fields */
#define CMD_STATUS_SUCCESS                 0
#define CMD_STATUS_ERROR_GENERAL           1
#define CMD_STATUS_ERROR_IF_OUT_OF_RANGE   2
#define CMD_STATUS_ERROR_IF_NOT_PRESENT    3

typedef struct sCommandHeader {
    u16  uCommandType;
    u16  uSequenceNumber;
} sCommandHeaderT;

typedef struct sGetEmbeddedSoftwareVersionReq {
  sCommandHeaderT Header;
} sGetEmbeddedSoftwareVersionReqT;

typedef struct sGetEmbeddedSoftwareVersionResp {
  sCommandHeaderT Header;
  u16       uVersionMajor;
  u16       uVersionMinor;
  u16       uVersionPatch;
  u16       uQSFPBootloaderVersionMajor;
  u16       uQSFPBootloaderVersionMinor;
  u16       uPadding[4];
} sGetEmbeddedSoftwareVersionRespT;

typedef struct sWriteRegReq {
  sCommandHeaderT Header;
    u16       uBoardReg;
    u16       uRegAddress;
    u16       uRegDataHigh;
    u16       uRegDataLow;
} sWriteRegReqT;

typedef struct sWriteRegResp {
  sCommandHeaderT Header;
    u16       uBoardReg;
    u16       uRegAddress;
    u16       uRegDataHigh;
    u16       uRegDataLow;
    u16       uPadding[5];
} sWriteRegRespT;

typedef struct sReadRegReq {
  sCommandHeaderT Header;
    u16       uBoardReg;
    u16       uRegAddress;
} sReadRegReqT;

typedef struct sReadRegResp {
  sCommandHeaderT Header;
    u16       uBoardReg;
    u16       uRegAddress;
    u16       uRegDataHigh;
    u16       uRegDataLow;
    u16       uPadding[5];
} sReadRegRespT;

typedef struct sWriteWishboneReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uWriteDataHigh;
    u16       uWriteDataLow;
} sWriteWishboneReqT;

typedef struct sWriteWishboneResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uWriteDataHigh;
    u16       uWriteDataLow;
    u16       uErrorStatus;     /* '0' = success, '1' = error i.e. out-of-range wishbone addr. This convention used
                                   since we have repurposed one of the padding bytes to serve as an error signal. A zero
                                   is returned as a 'success' in order to maintain backward compatibility, thus if a
                                   newer version of casperfpga interacts with an older microblaze, a zero would be
                                   returned and not block successful transactions. */
    u16       uPadding[4];
} sWriteWishboneRespT;

typedef struct sReadWishboneReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
} sReadWishboneReqT;

typedef struct sReadWishboneResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uReadDataHigh;
    u16       uReadDataLow;
    u16       uErrorStatus;     /* '0' = success, '1' = error i.e. out-of-range wishbone addr. This convention used
                                   since we have repurposed one of the padding bytes to serve as an error signal. A zero
                                   is returned as a 'success' in order to maintain backward compatibility, thus if a
                                   newer version of casperfpga interacts with an older microblaze, a zero would be
                                   returned and not block successful transactions. */
    u16       uPadding[4];
} sReadWishboneRespT;

#define MAX_I2C_WRITE_BYTES 33

typedef struct sWriteI2CReq {
  sCommandHeaderT Header;
    u16       uId;
    u16       uSlaveAddress;
    u16       uNumBytes;
    u16       uWriteBytes[MAX_I2C_WRITE_BYTES];
} sWriteI2CReqT;


typedef struct sWriteI2CResp {
  sCommandHeaderT Header;
    u16       uId;
    u16       uSlaveAddress;
    u16       uNumBytes;
    u16       uWriteBytes[MAX_I2C_WRITE_BYTES];
    u16       uWriteSuccess;
    //u16       uPadding[1];
} sWriteI2CRespT;

typedef struct sReadI2CReq {
  sCommandHeaderT Header;
    u16       uId;
    u16       uSlaveAddress;
    u16       uNumBytes;
} sReadI2CReqT;

typedef struct sReadI2CResp {
  sCommandHeaderT Header;
    u16       uId;
    u16       uSlaveAddress;
    u16       uNumBytes;
    u16       uReadBytes[32];
    u16       uReadSuccess;
    u16       uPadding[1];
} sReadI2CRespT;

typedef struct sSdramReconfigureReq {
  sCommandHeaderT Header;
  u16       uOutputMode;
  u16       uClearSdram;
  u16       uFinishedWritingToSdram;
  u16       uAboutToBootFromSdram;
  u16       uDoReboot;
  u16       uResetSdramReadAddress;
  u16       uClearEthernetStatistics;
  u16       uEnableDebugSdramReadMode;
  u16       uDoSdramAsyncRead;
  u16       uDoContinuityTest;
  u16       uContinuityTestOutputLow;
  u16       uContinuityTestOutputHigh;
} sSdramReconfigureReqT;

typedef struct sSdramReconfigureResp {
  sCommandHeaderT Header;
  u16       uOutputMode;
  u16       uClearSdram;
  u16       uFinishedWritingToSdram;
  u16       uAboutToBootFromSdram;
  u16       uDoReboot;
  u16       uResetSdramReadAddress;
  u16       uClearEthernetStatistics;
  u16       uEnableDebugSdramReadMode;
  u16       uDoSdramAsyncRead;
  u16       uNumEthernetFrames;
  u16       uNumEthernetBadFrames;
  u16       uNumEthernetOverloadFrames;
  u16       uSdramAsyncReadDataHigh;
  u16       uSdramAsyncReadDataLow;
  u16       uDoContinuityTest;
  u16       uContinuityTestReadLow;
  u16       uContinuityTestReadHigh;
} sSdramReconfigureRespT;

typedef struct sReadFlashWordsReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumWords;
} sReadFlashWordsReqT;

typedef struct sReadFlashWordsResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumWords;
    u16       uReadWords[384];
    u16       uPadding[2];
} sReadFlashWordsRespT;

typedef struct sProgramFlashWordsReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uTotalNumWords;
    u16       uNumWords;
    u16       uDoBufferedProgramming;
    u16       uStartProgram;
    u16       uFinishProgram;
    u16       uWriteWords[256];
} sProgramFlashWordsReqT;

typedef struct sProgramFlashWordsResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uTotalNumWords;
    u16       uNumWords;
    u16       uDoBufferedProgramming;
    u16       uStartProgram;
    u16       uFinishProgram;
    u16       uProgramSuccess;
    u16       uPadding[1];
} sProgramFlashWordsRespT;

typedef struct sEraseFlashBlockReq {
  sCommandHeaderT Header;
    u16       uBlockAddressHigh;
    u16       uBlockAddressLow;
} sEraseFlashBlockReqT;

typedef struct sEraseFlashBlockResp {
  sCommandHeaderT Header;
    u16       uBlockAddressHigh;
    u16       uBlockAddressLow;
    u16       uEraseSuccess;
    u16       uPadding[6];
} sEraseFlashBlockRespT;

typedef struct sReadSpiPageReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumBytes;
} sReadSpiPageReqT;

typedef struct sReadSpiPageResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumBytes;
    u16       uReadBytes[264];
    u16       uReadSpiPageSuccess;
    u16       uPadding[1];
} sReadSpiPageRespT;

typedef struct sProgramSpiPageReq {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumBytes;
    u16       uWriteBytes[264];
} sProgramSpiPageReqT;

typedef struct sProgramSpiPageResp {
  sCommandHeaderT Header;
    u16       uAddressHigh;
    u16       uAddressLow;
    u16       uNumBytes;
    u16       uVerifyBytes[264];
    u16       uProgramSpiPageSuccess;
    u16       uPadding[1];
} sProgramSpiPageRespT;

typedef struct sEraseSpiSectorReq {
  sCommandHeaderT Header;
    u16       uSectorAddressHigh;
    u16       uSectorAddressLow;
} sEraseSpiSectorReqT;

typedef struct sEraseSpiSectorResp {
  sCommandHeaderT Header;
    u16       uSectorAddressHigh;
    u16       uSectorAddressLow;
    u16       uEraseSuccess;
    u16       uPadding[6];
} sEraseSpiSectorRespT;

typedef struct sOneWireReadROMReq {
  sCommandHeaderT Header;
    u16       uOneWirePort;
} sOneWireReadROMReqT;

typedef struct sOneWireReadROMResp {
  sCommandHeaderT Header;
    u16       uOneWirePort;
    u16       uRom[8];
    u16       uReadSuccess;
    u16       uPadding[3];
} sOneWireReadROMRespT;

typedef struct sOneWireDS2433WriteMemReq {
  sCommandHeaderT Header;
  u16       uDeviceRom[8];
  u16       uSkipRomAddress;
  u16       uWriteBytes[32];
  u16       uNumBytes;
  u16       uTA1;
  u16       uTA2;
    u16       uOneWirePort;
} sOneWireDS2433WriteMemReqT;

typedef struct sOneWireDS2433WriteMemResp {
  sCommandHeaderT Header;
  u16       uDeviceRom[8];
  u16       uSkipRomAddress;
  u16       uWriteBytes[32];
  u16       uNumBytes;
  u16       uTA1;
  u16       uTA2;
    u16       uOneWirePort;
    u16       uWriteSuccess;
    u16       uPadding[3];
} sOneWireDS2433WriteMemRespT;

typedef struct sOneWireDS2433ReadMemReq {
  sCommandHeaderT Header;
  u16       uDeviceRom[8];
  u16       uSkipRomAddress;
  u16       uNumBytes;
  u16       uTA1;
  u16       uTA2;
    u16       uOneWirePort;
} sOneWireDS2433ReadMemReqT;

typedef struct sOneWireDS2433ReadMemResp {
  sCommandHeaderT Header;
  u16       uDeviceRom[8];
  u16       uSkipRomAddress;
  u16       uReadBytes[32];
  u16       uNumBytes;
  u16       uTA1;
  u16       uTA2;
    u16       uOneWirePort;
    u16       uReadSuccess;
    u16       uPadding[3];
} sOneWireDS2433ReadMemRespT;

typedef struct sDebugConfigureEthernetReq {
  sCommandHeaderT Header;
  u16       uId;
  u16       uFabricMacHigh;
  u16       uFabricMacMid;
  u16       uFabricMacLow;
  u16       uFabricPortAddress;
  u16       uGatewayIPAddressHigh;
  u16       uGatewayIPAddressLow;
  u16       uFabricIPAddressHigh;
  u16       uFabricIPAddressLow;
  u16       uFabricMultiCastIPAddressHigh;
  u16       uFabricMultiCastIPAddressLow;
  u16       uFabricMultiCastIPAddressMaskHigh;
  u16       uFabricMultiCastIPAddressMaskLow;
  u16       uEnableFabricInterface;
} sDebugConfigureEthernetReqT;

typedef struct sDebugConfigureEthernetResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uFabricMacHigh;
  u16       uFabricMacMid;
  u16       uFabricMacLow;
  u16       uFabricPortAddress;
  u16       uGatewayIPAddressHigh;
  u16       uGatewayIPAddressLow;
  u16       uFabricIPAddressHigh;
  u16       uFabricIPAddressLow;
  u16       uFabricMultiCastIPAddressHigh;
  u16       uFabricMultiCastIPAddressLow;
  u16       uFabricMultiCastIPAddressMaskHigh;
  u16       uFabricMultiCastIPAddressMaskLow;
  u16       uEnableFabricInterface;
  u16       uPadding[3];
} sDebugConfigureEthernetRespT;

typedef struct sDebugAddARPCacheEntryReq {
  sCommandHeaderT Header;
  u16       uId;
  u16       uIPAddressLower8Bits;
  u16       uMacHigh;
  u16       uMacMid;
  u16       uMacLow;
} sDebugAddARPCacheEntryReqT;

typedef struct sDebugAddARPCacheEntryResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uIPAddressLower8Bits;
  u16       uMacHigh;
  u16       uMacMid;
  u16       uMacLow;
  u16       uPadding[4];
} sDebugAddARPCacheEntryRespT;

typedef struct sPMBusReadI2CBytesReq {
  sCommandHeaderT Header;
  u16       uId;
  u16       uSlaveAddress;
  u16       uCommandCode;
  u16       uReadBytes[32];
  u16       uNumBytes;
} sPMBusReadI2CBytesReqT;

typedef struct sPMBusReadI2CBytesResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uSlaveAddress;
  u16       uCommandCode;
  u16       uReadBytes[32];
  u16       uNumBytes;
    u16       uReadSuccess;
} sPMBusReadI2CBytesRespT;

typedef struct sConfigureMulticastReq {
  sCommandHeaderT Header;
  u16       uId;                                /* set to interface identifier to be configured, or
                                                 * set to 0xff to configure on the interface the
                                                 * command is received on.
                                                 */
  u16       uFabricMultiCastIPAddressHigh;
  u16       uFabricMultiCastIPAddressLow;
  u16       uFabricMultiCastIPAddressMaskHigh;
  u16       uFabricMultiCastIPAddressMaskLow;
} sConfigureMulticastReqT;

typedef struct sConfigureMulticastResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uFabricMultiCastIPAddressHigh;
  u16       uFabricMultiCastIPAddressLow;
  u16       uFabricMultiCastIPAddressMaskHigh;
  u16       uFabricMultiCastIPAddressMaskLow;
  u16       uStatus;          /* set according to CMD_STATUS_* macros  */
  u16       uPadding[3];
} sConfigureMulticastRespT;

typedef struct sDebugLoopbackTestReq {
  sCommandHeaderT Header;
  u16       uId;
  u16       uTestData[256];
} sDebugLoopbackTestReqT;

typedef struct sDebugLoopbackTestResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uTestData[256];
  u16       uValid;
  u16       uPadding[3];
} sDebugLoopbackTestRespT;

typedef struct sQSFPResetAndProgramReq {
  sCommandHeaderT Header;
    u16       uReset;
    u16       uProgram;
} sQSFPResetAndProgramReqT;

typedef struct sQSFPResetAndProgramResp {
  sCommandHeaderT Header;
    u16       uReset;
    u16       uProgram;
  u16       uPadding[7];
} sQSFPResetAndProgramRespT;

typedef struct sHMCReadI2CBytesReq {
  sCommandHeaderT Header;
  u16       uId;
  u16       uSlaveAddress;
  u16       uReadAddress[4];
} sHMCReadI2CBytesReqT;

typedef struct sHMCReadI2CBytesResp {
  sCommandHeaderT Header;
  u16       uId;
  u16       uSlaveAddress;
  u16       uReadAddress[4];
  u16       uReadBytes[4];
  u16       uReadSuccess;
  u16       uPadding[2];
} sHMCReadI2CBytesRespT;

typedef struct sHMCWriteI2CBytesReq {
  sCommandHeaderT Header;
  u16 uId;
  u16 uSlaveAddress;
  u16 uWriteAddress[4];
  u16 uWriteData[4];
} sHMCWriteI2CBytesReqT;

typedef struct sHMCWriteI2CBytesResp {
  sCommandHeaderT Header;
  u16 uId;
  u16 uSlaveAddress;
  u16 uWriteAddress[4];
  u16 uWriteData[4];
  u16 uWriteSuccess;
  u16 uPadding[2];
} sHMCWriteI2CBytesRespT;

/* new SDRAM programming request / response */
#define CHUNK_SIZE  994   /* amount of 16-bit words per chunk */
//#define CHUNK_SIZE  1988   /* amount of 16-bit words per chunk */

typedef struct sSDRAMProgramOverWishboneReq {
  sCommandHeaderT Header;
  u16 uChunkNum;
  u16 uChunkTotal;
  u16 uBitstreamChunk[];
} sSDRAMProgramOverWishboneReqT;

typedef struct sSDRAMProgramOverWishboneResp {
  sCommandHeaderT Header;
  u16 uChunkNum;
  u16 uStatus;
  u16 uPadding[7];
} sSDRAMProgramOverWishboneRespT;

typedef struct sSetDHCPTuningDebugReq {
  sCommandHeaderT Header;
  u16 uInitTime;
  u16 uRetryTime;
} sSetDHCPTuningDebugReqT;

typedef struct sSetDHCPTuningDebugResp {
  sCommandHeaderT Header;
  u16 uInitTime;
  u16 uRetryTime;
  u16 uStatus;
  u16 uPadding[6];
} sSetDHCPTuningDebugRespT;

typedef struct sGetDHCPTuningDebugReq {
  sCommandHeaderT Header;
} sGetDHCPTuningDebugReqT;

typedef struct sGetDHCPTuningDebugResp {
  sCommandHeaderT Header;
  u16 uInitTime;
  u16 uRetryTime;
  u16 uStatus;
  u16 uPadding[6];
} sGetDHCPTuningDebugRespT;

typedef struct sLogDataEntry{
  u16 uPageSpecific;
  u16 uFaultType;
  u16 uPage;
  u16 uFaultValue;
  u16 uValueScale;
  u16 uSeconds[2];     /* most significant word first - runtime seconds since fault */
#if 0
  u16 uMilliSeconds[2];   /* most-significant word first */
  u16 uDays[2];           /* most-significant word first */
#endif
} sLogDataEntryT;

typedef struct sGetCurrentLogsReq {
  sCommandHeaderT Header;
} sGetCurrentLogsReqT;

typedef struct sGetCurrentLogsResp {
  sCommandHeaderT Header;
#define NUM_LOG_ENTRIES 16
  sLogDataEntryT uCurrentLogEntries[NUM_LOG_ENTRIES];
  u16 uLogEntrySuccess;
} sGetCurrentLogsRespT;

/* TODO: can't we use a union to collapse these two identical structs? (above and below) */

typedef struct sGetVoltageLogsReq {
  sCommandHeaderT Header;
} sGetVoltageLogsReqT;

typedef struct sGetVoltageLogsResp {
  sCommandHeaderT Header;
#define NUM_LOG_ENTRIES 16
  sLogDataEntryT uVoltageLogEntries[NUM_LOG_ENTRIES];
  u16 uLogEntrySuccess;
} sGetVoltageLogsRespT;

typedef struct sFanCtrlrLogDataEntry {
  u16 uFaultLogIndex;
  u16 uFaultLogCount;
  u16 uStatusWord;
  u16 uStatusVout_17_18;
  u16 uStatusVout_19_20;
  u16 uStatusVout_21_22;
  u16 uStatusMfr_6_7;
  u16 uStatusMfr_8_9;
  u16 uStatusMfr_10_11;
  u16 uStatusMfr_12_13;
  u16 uStatusMfr_14_15;
  u16 uStatusMfr_16_none;
  u16 uStatusFans_0_1;
  u16 uStatusFans_2_3;
  u16 uStatusFans_4_5;
} sFanCtrlrLogDataEntryT;

typedef struct sGetFanControllerLogsReq {
  sCommandHeaderT Header;
} sGetFanControllerLogsReqT;

#define NUM_FANCTRLR_LOG_ENTRIES  15
typedef struct sGetFanControllerLogsResp {
  sCommandHeaderT Header;
  sFanCtrlrLogDataEntryT uFanCtrlrLogEntries[NUM_FANCTRLR_LOG_ENTRIES];
  u16 uCompleteSuccess;      /* true if full log retrieval is successful */
  u16 uPadding[3];
} sGetFanControllerLogsRespT;

typedef struct sClearFanControllerLogsReq {
  sCommandHeaderT Header;
} sClearFanControllerLogsReqT;

typedef struct sClearFanControllerLogsResp {
  sCommandHeaderT Header;
  u16 uSuccess;      /* true if logs successfully cleared */
  u16 uPadding[8];
} sClearFanControllerLogsRespT;

typedef struct sDHCPResetStateMachineReq {
  sCommandHeaderT Header;
  u16 uLinkId;    /* this is the id of the interface
                   * we want to reset dhcp on
                   * e.g. 1 for 40gbe on site, 0 for 1gbe
                   */
} sDHCPResetStateMachineReqT;

typedef struct sDHCPResetStateMachineResp {
  sCommandHeaderT Header;
  u16 uLinkId;
  u16 uResetError;
  /* uResetError:
   * 0 => no errors
   * 1 => link non-existent
   * 2 => link currently down
   */
  u16 uPadding[7];
} sDHCPResetStateMachineRespT;

/* TODO */
typedef struct sMulticastLeaveGroupReq {
  sCommandHeaderT Header;
  u16 uLinkId;    /* this is the id of the interface */
} sMulticastLeaveGroupReqT;

typedef struct sMulticastLeaveGroupResp {
  sCommandHeaderT Header;
  u16 uLinkId;
  u16 uSuccess;
  u16 uPadding[7];
} sMulticastLeaveGroupRespT;

typedef struct sGetDHCPMonitorTimeoutReq {
  sCommandHeaderT Header;
} sGetDHCPMonitorTimeoutReqT;

typedef struct sGetDHCPMonitorTimeoutResp {
  sCommandHeaderT Header;
  u16 uDHCPMonitorTimeout;
  u16 uPadding[8];
} sGetDHCPMonitorTimeoutRespT;

typedef struct sGetMicroblazeUptimeReq {
  sCommandHeaderT Header;
} sGetMicroblazeUptimeReqT;

typedef struct sGetMicroblazeUptimeResp {
  sCommandHeaderT Header;
  u16 uMicroblazeUptimeHigh;
  u16 uMicroblazeUptimeLow;
  u16 uPadding[7];
} sGetMicroblazeUptimeRespT;

/* update the MAX31785 fan controller parameters */
typedef struct sFPGAFanControllerUpdateReq {
  sCommandHeaderT Header;
  u16 uEnableFanControl;    /* 1=automatic fan control mode */
  u16 uUpdateSetpoints;
  u16 uSetpoints[16];     /* 8x (temperature,pwm) each setpoint data is 16-bit wide */
  u16 uWriteToFlash;        /* default=FALSE; TRUE=>this stores the currently received set of parameters
                               accompanying this command to flash  */
} sFPGAFanControllerUpdateReqT;

typedef struct sFPGAFanControllerUpdateResp {
  sCommandHeaderT Header;
  u16 uError;  /* status: send 0 for success, >= 1 for error */
  u16 uPadding[8];
} sFPGAFanControllerUpdateRespT;

typedef struct sGetFPGAFanControllerLUTReq {
  sCommandHeaderT Header;
} sGetFPGAFanControllerLUTReqT;

typedef struct sGetFPGAFanControllerLUTResp {
  sCommandHeaderT Header;
  u16 uSetpointData[16];
  u16 uError;  /* status: send 0 for success, >= 1 for error */
  u16 uPadding[4];
} sGetFPGAFanControllerLUTRespT;

typedef struct sADCMezzanineResetAndProgramReq {
  sCommandHeaderT Header;
  u16  			uReset;
  u16				uProgram;
  u16				uMezzanine;
} sADCMezzanineResetAndProgramReqT;

typedef struct sADCMezzanineResetAndProgramResp {
  sCommandHeaderT Header;
  u16  			uReset;
  u16				uProgram;
  u16				uMezzanine;
  u16				uPadding[6];
} sADCMezzanineResetAndProgramRespT;

// I2C BUS DEFINES
#define MB_I2C_BUS_ID       0x0
#define MEZZANINE_0_I2C_BUS_ID    0x1
#define MEZZANINE_1_I2C_BUS_ID    0x2
#define MEZZANINE_2_I2C_BUS_ID    0x3
#define MEZZANINE_3_I2C_BUS_ID    0x4

//STM I2C DEFINES
#define STM_I2C_DEVICE_ADDRESS    0x0C    // 0x18 shifted down by 1 bit
  
#define PCA9546_I2C_DEVICE_ADDRESS  0x70  // Address without read/write bit
#define FAN_CONT_SWITCH_SELECT    0x01
#define MONITOR_SWITCH_SELECT   0x02
#define ONE_GBE_SWITCH_SELECT   0x04

#define GBE_88E1111_I2C_DEVICE_ADDRESS  0x58  // Without read/write bit

#ifdef __cplusplus
}
#endif
#endif
