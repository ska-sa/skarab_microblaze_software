/**-----------------------------------------------------------------------------
 *FILE NAME            : main.c
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
 *  This file contains contains the top level functions for the ROACH3 uBlaze
 *  embedded processor.
 * ------------------------------------------------------------------------------*/

#include <stdio.h>
#include <xparameters.h>
#include <xil_cache.h>
#include <xil_assert.h>
#include <xil_io.h>
#include <xintc.h>
#include <xwdttb.h>
#include <xtmrctr.h>
#include <mb_interface.h>
#include <xuartlite.h>

#include "register.h"
#include "delay.h"
#include "eth_mac.h"
#include "flash_sdram_controller.h"
#include "icape_controller.h"
#include "isp_spi_controller.h"
#include "one_wire.h"
#include "i2c_master.h"
#include "constant_defs.h"
#include "eth_sorter.h"
#include "sensors.h"
#include "improved_read_write.h"
#include "dhcp.h"
#include "lldp.h"
#include "icmp.h"
#include "net_utils.h"
#include "if.h"
#include "arp.h"
#include "memtest.h"
#include "diagnostics.h"
#include "scratchpad.h"
#include "qsfp.h"
#include "wdt.h"
#include "id.h"
#include "adc.h"
#include "mezz.h"
#include "logging.h"
#include "cli.h"
#include "fault_log.h"
#include "igmp.h"
#include "time.h"
#include "init.h"
#include "error.h"

#define DHCP_MAX_RECONFIG_COUNT 2

#if defined(LINK_MON_RX_40GBE) && defined(RECONFIG_UPON_NO_DHCP)
#error "Cannot enable both link monitoring and dhcp unbound monitoring tasks - edit in Makefile.config"
#endif

#define LINK_MON_COUNTER_DEFAULT_VALUE  600
#define LINK_MON_COUNTER_MIN_VALUE  600

#define DHCP_MON_COUNTER_DEFAULT_VALUE  450 /* 45 seconds */
#define DHCP_MON_COUNTER_MIN_VALUE  50    /* 5 seconds */

/* local function prototypes */
static int vSendDHCPMsg(struct sIFObject *pIFObjectPtr, void *pUserData);
static int vSetInterfaceConfig(struct sIFObject *pIFObjectPtr, void *pUserData);

void DivByZeroException(void *Data);
void IBusException(void *Data);
void DBusException(void *Data);
void StackViolationException(void *Data);
void IllegalOpcodeException(void *Data);
/* void UnalignedAccessException(void *Data); */

/* temp global definition */
static volatile u8 uFlagRunTask_DHCP[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_IGMP[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_LLDP[NUM_ETHERNET_INTERFACES] = {0};
#ifdef RECONFIG_UPON_NO_DHCP
static volatile u8 uFlagRunTask_CheckDHCPBound = 0;
static volatile u8 uFlagRunTask_LinkRecovery = 0;
#endif
static volatile u8 uFlagRunTask_ICMP_Reply[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_ARP_Process[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_ARP_Respond[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_CTRL[NUM_ETHERNET_INTERFACES] = {0};
static volatile u8 uFlagRunTask_Diagnostics = 0;
static volatile u8 uFlagRunTask_ARP_Requests[NUM_ETHERNET_INTERFACES] = {ARP_REQUEST_DONT_UPDATE};
static volatile u8 uFlagRunTask_WriteLEDStatus = 0;
#ifdef LINK_MON_RX_40GBE
static volatile u8 uFlagRunTask_LinkWatchdog = 0;
#endif

volatile u8 uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;

static volatile u8 uFlagRunTask_Aux = 0;

static volatile u16 uTimeoutCounter = 255;
static volatile u8 uTick_100ms = 0;

//u32 __attribute__ ((section(".rodata.memtest"))) __attribute__ ((aligned (32))) uMemPlace = 0xAABBCCDD ;

/* place this variable at a specific location in memory (in .rodata section). See linker script */
//u32 __attribute__ ((section(".rodata.checksum"))) __attribute__ ((aligned (32))) uMemPlace;

#if 0
/* this variable holds the end address of the .text section. See linker script */
extern u32 _text_section_end_;
/* this variable holds the location at which the externally computed checksum was stored by linker. See linker script */
extern u32 _location_checksum_;
#endif

//static struct sDHCPObject DHCPContextState[NUM_ETHERNET_INTERFACES];  /* TODO can we narrow down the scope of this data? */
//static struct sICMPObject ICMPContextState[NUM_ETHERNET_INTERFACES];
//static struct sIFObject IFContext[NUM_ETHERNET_INTERFACES];
//static struct sQSFPObject QSFPContext;
//static struct sAdcObject AdcContext;
//static struct sQSFPObject *QSFPContext_hdl;
//static struct sAdcObject *AdcContext_hdl;

static struct sMezzObject *MezzHandle[4];   /* holds the handle to the state of each mezzanine card*/

//=================================================================================
//  TimerHandler
//--------------------------------------------------------------------------------
//  This method handles the interrupts generated by the Timer.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  CallBackRef     IN  Reference to timer object
//  uTimerCounterNumber IN  There are two timers inside the timer object
//
//  Return
//  ------
//  None
//=================================================================================
void TimerHandler(void * CallBackRef, u8 uTimerCounterNumber)
{
  //u8 uIndex;
  int i;

	XTmrCtr *InstancePtr = (XTmrCtr *)CallBackRef;

	if (XTmrCtr_IsExpired(InstancePtr, DHCP_RETRY_TIMER_ID)) {
    if (uTimeoutCounter != 0){
      uTimeoutCounter--;
    }

    uTick_100ms = 1;

    /* every 5 mins */
    if (uPrintStatsCounter >= 3000){
      uFlagRunTask_Diagnostics = 1;
      uPrintStatsCounter = 0;
    } else {
      uPrintStatsCounter++;
    }

    // LLDP every 15 seconds (timer every 100 ms)
    if(uLLDPTimerCounter >= 150){
      uLLDPTimerCounter = 0x0;
      for (i = 0; i < NUM_ETHERNET_INTERFACES; i++){
        uFlagRunTask_LLDP[i] = 1;
      }
    } else{
      uLLDPTimerCounter++;
    }

    for (i = 0; i < NUM_ETHERNET_INTERFACES; i++){
      /* set the dhcp task flag every 100ms which in turn runs dhcp state machine */
      uFlagRunTask_DHCP[i] = 1;

      /* set the igmp task flag every 100ms which in turn runs igmp state machine */
      uFlagRunTask_IGMP[i] = 1;

      // Every 100 ms, send another ARP request
      uFlagRunTask_ARP_Requests[i] = ARP_REQUEST_UPDATE;
    }

#ifdef RECONFIG_UPON_NO_DHCP
    uFlagRunTask_CheckDHCPBound = 1;
    uFlagRunTask_LinkRecovery = 1;
#endif

#ifdef LINK_MON_RX_40GBE
    uFlagRunTask_LinkWatchdog = 1;
#endif

    uQSFPUpdateStatusEnable = UPDATE_QSFP_STATUS;
    uADC32RF45X2UpdateStatusEnable = UPDATE_ADC32RF45X2_STATUS;

    uFlagRunTask_WriteLEDStatus = 1;
  }

	if (XTmrCtr_IsExpired(InstancePtr, AUX_RETRY_TIMER_ID)) {
    uFlagRunTask_Aux = 1;
  }
}



//=================================================================================
//  EthernetRecvHandler
//--------------------------------------------------------------------------------
//  This method processes a packet stored in uReceiveBuffer.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId     IN    ID of network interface that packet was received from
//  uNumWords IN    Packet size in 32 bit words
//  uResponsePacketLength OUT Length of response packet in bytes
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int EthernetRecvHandler(u8 uId, u32 uNumWords, u32 * uResponsePacketLengthBytes)
{
  // L2 - Ethernet
  // L3 - IP(v4) / ARP
  // L4 - UDP
  // L5 - command protocol
  u32 uL2PktLen;
  u32 uL3PktLen;
  u32 uL4PktLen;
  u32 uL5PktLen;
  u8 *pL2Ptr;
  u8 *pL3Ptr;
  u8 *pL4Ptr;
  u8 *pL5Ptr;
  u8 *uResponsePacketPtr;
  u32 uResponseLength;

  uResponsePacketPtr = (u8*) (uTransmitBuffer);
  uResponsePacketPtr = uResponsePacketPtr + sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT);

  struct sEthernetHeader *EthHdr;
  u32 uL3Proto;
  u32 uL3TOS;
  u32 uUdpSrcPort;
  u32 uUdpDstPort;
  int iStatus;
  struct sIFObject *pIFObjectPtr;

  /* get the handle to the interface context state */
  pIFObjectPtr = lookup_if_handle_by_id(uId);

  // Check we have enough data to proceed: i.e. at least valid Ethernet header
  uL2PktLen = uNumWords * 4;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Ethernet Receive Handler\r\n");

  if (uL2PktLen < sizeof(struct sEthernetHeader))
    return XST_FAILURE;

  //pL2Ptr = (u8 *) &(uReceiveBuffer[uId][0]);
  pL2Ptr = (u8 *) uReceiveBuffer;
  EthHdr = (struct sEthernetHeader *) pL2Ptr;

  // Cache MAC source address for response
  uResponseMacHigh = EthHdr->uSourceMacHigh;
  uResponseMacMid = EthHdr->uSourceMacMid;
  uResponseMacLow = EthHdr->uSourceMacLow;

  pL3Ptr = pL2Ptr + sizeof(struct sEthernetHeader);
  uL3PktLen = uL2PktLen - sizeof(struct sEthernetHeader);

  // TO DO: HANDLE ALL NON-IP/UDP PACKETS HERE
#if 0
  if (EthHdr->uEthernetType == ETHERNET_TYPE_ARP){
    iStatus = CheckArpRequest(uId, uEthernetFabricIPAddress[uId], uL3PktLen, pL3Ptr);
    if (iStatus == XST_SUCCESS){
#ifdef DEBUG_PRINT
      //log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "ARP packet received!\r\n");
#endif
      ArpHandler(uId, ARP_RESPONSE, pL3Ptr, (u8 *) uTransmitBuffer, uResponsePacketLengthBytes, 0x0);
      return XST_SUCCESS;
    } else {
      return XST_FAILURE;
    }
  } else if (EthHdr->uEthernetType == ETHERNET_TYPE_IPV4) {
#endif
  if (EthHdr->uEthernetType == ETHERNET_TYPE_IPV4) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "IP packet received!\r\n");
    iStatus = CheckIPV4Header(pIFObjectPtr->uIFAddrIP, pIFObjectPtr->uIFAddrMask, uL3PktLen, pL3Ptr);
    if (iStatus == XST_SUCCESS){
      pL4Ptr = ExtractIPV4FieldsAndGetPayloadPointer(pL3Ptr, &uL4PktLen, (u32 *) &uResponseIPAddr, &uL3Proto, &uL3TOS);
      if (uL3Proto == IP_PROTOCOL_UDP){
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "UDP packet received!\r\n");
        iStatus = CheckUdpHeader(pL3Ptr, uL4PktLen, pL4Ptr);
        if (iStatus == XST_SUCCESS){
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "UDP packet success!\r\n");
          pL5Ptr = ExtractUdpFieldsAndGetPayloadPointer(pL4Ptr,& uL5PktLen, & uUdpSrcPort, & uUdpDstPort);
          uResponseUDPPort = uUdpSrcPort;
          // Command protocol
          if (uUdpDstPort == ETHERNET_CONTROL_PORT_ADDRESS){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Control packet received!\r\n");
            iStatus = CommandSorter(uId, pL5Ptr, uL5PktLen, uResponsePacketPtr, & uResponseLength);
            if (iStatus == XST_SUCCESS){
              log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Control packet success\r\n");
              // Create rest of Ethernet packet in transmit buffer for response
              CreateResponsePacket(uId, (u8 *) uTransmitBuffer, uResponseLength);
              * uResponsePacketLengthBytes = (uResponseLength + sizeof(sEthernetHeaderT) + sizeof(sIPV4HeaderT) + sizeof(sUDPHeaderT));
              return XST_SUCCESS;
            } else {
              return XST_FAILURE;
            }
          } else {
            return XST_FAILURE;
          }
        } else {
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "check UDP header failed: src mac %04x.%04x.%04x\r\n", EthHdr->uSourceMacHigh, EthHdr->uSourceMacMid, EthHdr->uSourceMacLow);
          return XST_FAILURE;
        }
      } else {
        return XST_FAILURE;
      }
    } else {
      return XST_FAILURE;
    }
  } else {
    return XST_FAILURE;
  }

}



//=================================================================================
//  InitialiseEthernetInterfaceParameters
//--------------------------------------------------------------------------------
//  This method configures the Ethernet interfaces with some default values.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void InitialiseEthernetInterfaceParameters()
{
  u8 logical_link;
  u8 uPhysicalEthernetId;
  u8 uSerial[ID_SK_SERIAL_LEN];

  u16 uFabricMacHigh;
  u16 uFabricMacMid;
  u16 uFabricMacLow;
  u8 status;
  u8 n;

  uPreviousAsyncSdramRead = 0;
  uPreviousSequenceNumber = 0;

  status = get_skarab_serial(uSerial, ID_SK_SERIAL_LEN);
  if (status == XST_FAILURE){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Failed to read SKARAB serial number.\r\n");
    /* from here on in the mac address will be incorrect (ff:ff:ff...) */
  }


  // TO DO: NOT SURE WHAT TO DO HERE FOR MULTICAST
  uFabricMacHigh = 0x0600 | uSerial[0];
  uFabricMacMid = (uSerial[1] << 8) | uSerial[2];

  n = get_num_interfaces();

  for (logical_link = 0; logical_link < n; logical_link++){

    uPhysicalEthernetId = get_physical_interface_id(logical_link);

    uIPIdentification[uPhysicalEthernetId] = 0x7342;

#ifdef DO_40GBE_LOOPBACK_TEST
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Setting all 40GBE MAC addresses same so can test in loopback!\r\n");
    if (uPhysicalEthernetId == 0){
      uFabricMacLow = (uSerial[3] << 8) | uPhysicalEthernetId;
    } else {
      uFabricMacLow = (uSerial[3] << 8) | 0x1;
    }
#else
    uFabricMacLow = (uSerial[3] << 8) | uPhysicalEthernetId;
#endif

    uEthernetFabricMacHigh[uPhysicalEthernetId] = uFabricMacHigh;
    uEthernetFabricMacMid[uPhysicalEthernetId] = uFabricMacMid;
    uEthernetFabricMacLow[uPhysicalEthernetId] = uFabricMacLow;

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] Setting MAC for IF %d to %04x %04x %04x\r\n", uPhysicalEthernetId, uPhysicalEthernetId, uFabricMacHigh, uFabricMacMid, uFabricMacLow);
    SetFabricSourceMACAddress(uPhysicalEthernetId, uEthernetFabricMacHigh[uPhysicalEthernetId], ((uEthernetFabricMacMid[uPhysicalEthernetId] << 16) | (uEthernetFabricMacLow[uPhysicalEthernetId])));

    status = SoftReset(uPhysicalEthernetId);
    if (status == XST_FAILURE){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "I/F  [%02x] Failed to do soft reset\r\n", uPhysicalEthernetId);
    } else {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "I/F  [%02x] Soft reset successful.\r\n", uPhysicalEthernetId);
    }
  }

}

//=================================================================================
//  InitialiseMezzanineLocations
//--------------------------------------------------------------------------------
//  This method detects the Mezzanine cards (QSFP+, ADC or HMC).
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
static void InitialiseMezzanineLocations()
{
  u8 mezz;

  /* should this not move to the interface init? */
  uQSFPCtrlReg = QSFP0_RESET | QSFP1_RESET | QSFP2_RESET | QSFP3_RESET;
  WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);

  for (mezz = 0; mezz < 4; mezz++){
    MezzHandle[mezz] = init_mezz_location(mezz);
  }

  if (uQSFPMezzaninePresent == QSFP_MEZZANINE_NOT_PRESENT){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to find a QSFP+ MEZZANINE!\r\n");
  }
}



//=================================================================================
//  InitialiseInterruptControllerAndTimer
//--------------------------------------------------------------------------------
//  This method configures the interrupt controller and timer used for DHCP timeouts.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void InitialiseInterruptControllerAndTimer(XTmrCtr * pTimer, XIntc * pInterruptController)
{
  u32 uDHCPTimerOptions;
  int iStatus;

  iStatus = XTmrCtr_Initialize(pTimer, XPAR_TMRCTR_0_DEVICE_ID);

  if (iStatus != XST_SUCCESS)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to initialise DHCP retry timer.\r\n");

  iStatus = XIntc_Initialize(pInterruptController, XPAR_INTC_SINGLE_DEVICE_ID);

  if (iStatus == XST_DEVICE_IS_STARTED)
  {
    XIntc_Stop(pInterruptController);
    iStatus = XIntc_Initialize(pInterruptController, XPAR_INTC_SINGLE_DEVICE_ID);
  }

  if (iStatus == XST_SUCCESS)
  {
    iStatus = XIntc_Connect(pInterruptController, XPAR_INTC_0_TMRCTR_0_VEC_ID, (XInterruptHandler) XTmrCtr_InterruptHandler, (void*) pTimer);

    if (iStatus == XST_SUCCESS)
    {
      iStatus =  XIntc_Start(pInterruptController, XIN_REAL_MODE);
      if (iStatus == XST_SUCCESS)
        XIntc_Enable(pInterruptController, XPAR_INTC_0_TMRCTR_0_VEC_ID);
      else
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to start interrupt controller.\r\n");
    }
    else
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to connect Timer interrupt to interrupt controller.\r\n");
  }
  else
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to initialise interrupt controller.\r\n");

  microblaze_enable_interrupts();

  XTmrCtr_SetHandler(pTimer, TimerHandler, pTimer);
  uDHCPTimerOptions = XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | XTC_DOWN_COUNT_OPTION;
  XTmrCtr_SetOptions(pTimer, DHCP_RETRY_TIMER_ID, uDHCPTimerOptions);
  XTmrCtr_SetResetValue(pTimer, DHCP_RETRY_TIMER_ID, DHCP_TIMER_RESET_VALUE);
  XTmrCtr_Start(pTimer, DHCP_RETRY_TIMER_ID);

  XTmrCtr_SetOptions(pTimer, AUX_RETRY_TIMER_ID, uDHCPTimerOptions);          /* same options as dhcp timer above */
  XTmrCtr_SetResetValue(pTimer, AUX_RETRY_TIMER_ID, AUX_TIMER_RESET_VALUE);

  /* uncomment to start the second timer and enable the associated interrupt and task */
  /* XTmrCtr_Start(pTimer, AUX_RETRY_TIMER_ID); */

}


int main()
{
  /* initialise globals TODO: narrow down scope */
  uQSFPMezzanineLocation = 0x0;
  uQSFPMezzaninePresent = QSFP_MEZZANINE_NOT_PRESENT;
  uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;
  uQSFPI2CMicroblazeAccess = QSFP_I2C_MICROBLAZE_ENABLE;

  //uADC32RF45X2MezzanineLocation = 0x0;
  //uADC32RF45X2MezzaninePresent = ADC32RF45X2_MEZZANINE_NOT_PRESENT;
  uADC32RF45X2UpdateStatusEnable = DO_NOT_UPDATE_ADC32RF45X2_STATUS;

  struct sIFObject *pIFObjectPtr[NUM_ETHERNET_INTERFACES]; /* store i/f state handles */
  int iStatus;
  u32 uReadReg;
  u8 uPhysicalEthernetId;
  u32 uNumWords;
  u32 uResponsePacketLength;
  XTmrCtr Timer;
  XIntc InterruptController;
  XWdtTb WatchdogTimer;

  XUartLite UartLite;		/* Instance of the UartLite Device */
  u8 RecvBuffer;
  u8 ReceivedCount = 0;


  //u32 uIGMPGroupAddress;
  u8 uOKToReboot;
#ifdef RECONFIG_UPON_NO_DHCP
  //u16 uDHCPBoundCount[NUM_ETHERNET_INTERFACES] = {0};
  //u8 uDHCPBoundTimeout = 0;
  u8 uDHCPReconfigCount = DHCP_MAX_RECONFIG_COUNT;
#endif
#ifdef DO_40GBE_LOOPBACK_TEST
  u32 uTemp40GBEIPAddress = 0x0A000802;
  u8 uConfig40GBE[4];
  uConfig40GBE[0] = 0x1;
  uConfig40GBE[1] = 0x1;
  uConfig40GBE[2] = 0x1;
  uConfig40GBE[3] = 0x1;
#endif

#ifdef DO_1GBE_LOOPBACK_TEST
  u32 uTemp1GBEIPAddress = 0x0A000702;
  u8 uConfig1GBE = 0x1;
#endif

  //struct sDHCPObject DHCPContextState[NUM_ETHERNET_INTERFACES];  /* TODO do we have enough stack space???*/

  /* Temp / Reused variables */
  u32 uSize = 0;       /* used to pass around / hold a packet size */
  u32 uIndex = 0;      /* used to index arrays / buffers */
  u16 *pBuffer = NULL;  /* pointer to working buffer */
  u16 uChecksum = 0;
  u32 uHMCBitMask;
  unsigned int uHMCRunInit = 0;

#define HMC_MAX_RECONFIG_COUNT 3
  u8 uHMC_Total_ReconfigCount = HMC_MAX_RECONFIG_COUNT;
  u8 uHMC_Max_ReconfigCount = HMC_MAX_RECONFIG_COUNT;
  u8 uHMC_ReconfigCount[4] = {0};

  u8 PMemState = PMEM_RETURN_ERROR;

  typePacketFilter uPacketType = PACKET_FILTER_UNKNOWN;
  u8 uValidate = 0;

  u32 uKeepAliveReg;

#ifdef TIME_PROFILE
  u32 time1 = 0, time2 = 0;
#endif

  u8 uFrontPanelLedsValue = 0;
  //u8 n_links;
  u8 num_links;
  u8 logical_link;

#ifdef LINK_MON_RX_40GBE
  u32 LinkTimeout = 1;
  u16 uLinkTimeoutMax = LINK_MON_COUNTER_DEFAULT_VALUE;
#endif

#ifdef RECONFIG_UPON_NO_DHCP
  u16 uDHCPTimeoutMax = DHCP_MON_COUNTER_DEFAULT_VALUE;
  tPMemReturn ret;
#endif

#if 0
#if defined(LINK_MON_RX_40GBE) || defined(RECONFIG_UPON_NO_DHCP)
  u16 uLinkTimeoutMax = LINK_MON_COUNTER_DEFAULT_VALUE;
#endif
#endif

  unsigned char *buf,temp;

  u16 rom[8];
  u16 data[4];    /* this variable reused to read eeprom data - note: only 4 bytes */
  u16 dhcp_wait = 0;
  u16 dhcp_retry = 0;
  u8 dhcp_set = 0;

  u16 uHMCTimeout;

  u8 post_scale = 0;

  /* need to cache the current reconfiguration location for later use - e.g. when issuing reboot from last location cmd
   * this read has to happen early in the boot seq before the value is cleared in firmware */
  u32 uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS);

  /*
   * Initialize the uart driver
   */
  iStatus = XUartLite_Initialize(&UartLite, XPAR_UARTLITE_0_DEVICE_ID);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UART [..] Unable to configure the UART driver\r\n");
  } else {
    /*
     * Perform a self-test to ensure that the hardware was built correctly.
     */
    iStatus = XUartLite_SelfTest(&UartLite);
    if (iStatus != XST_SUCCESS) {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UART [..] Seft-test FAILED!\r\n");
    } else {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "\r\n\r\nUART [..] Initialized successfully!\r\n");
    }
  }

  /* print only after the uart initialized */
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] C_CONFIG_IO_REG = 0x%08x\r\n", uReg);

  FinishedBootingFromSdram();

  CLI_STATE cli_status = cli_init();
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "CLI  [..] init status=%d\r\n", cli_status);

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] Waiting 0.5s...\r\n");
  Delay(500000);

  ReceivedCount = XUartLite_Recv(&UartLite, &RecvBuffer, 1);
  if (ReceivedCount){
    while (1){
      ReceivedCount = XUartLite_Recv(&UartLite, &RecvBuffer, 1);
      if (ReceivedCount){
        cli_sm(RecvBuffer);
      }
    }
  }

  vRunMemoryTest();

  PMemState = init_persistent_memory_setup();

  //Xil_ICacheEnable();
  //Xil_DCacheEnable();

  uDoReboot = NO_REBOOT;

  InitI2C(0x0, SPEED_100kHz);
  InitI2C(0x1, SPEED_400kHz);
  InitI2C(0x2, SPEED_400kHz);
  InitI2C(0x3, SPEED_400kHz);
  InitI2C(0x4, SPEED_400kHz);

  /* initialise the log level */
  u8 log_level_cached = 0;
  u8 log_select_cached = 0;
  tLogLevel log_level = LOG_LEVEL_INFO;
  tLogSelect log_select = LOG_SELECT_ALL;

  if (PMemState != PMEM_RETURN_ERROR){
    PersistentMemory_ReadByte(LOG_LEVEL_STARTUP_INDEX, &log_level_cached);
    PersistentMemory_ReadByte(LOG_SELECT_STARTUP_INDEX, &log_select_cached);

    /* if the msb is set, then we know that the log-level command was issued before the reset */
    if (log_level_cached & 0x80){
      /* the cached log-level is stored in the lower 7 bits */
      log_level = (tLogLevel) (log_level_cached & 0x7f);
    } else {
      /* otherwise default to *info* */
      log_level = LOG_LEVEL_INFO;
    }

    /* if the msb is set, then we know that the log-select command was issued before the reset */
    if (log_select_cached & 0x80){
      /* the cached log-select is stored in the lower 7 bits */
      log_select = (tLogSelect) (log_select_cached & 0x7f);
    } else {
      /* otherwise default to *all* */
      log_select = LOG_SELECT_ALL;
    }
  }

  set_log_level(log_level);
  set_log_select(log_select);

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "---Entering main---\r\n");
  PrintVersionInfo();

  init_wdt(&WatchdogTimer);

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Interrupts, exceptions and timers ");
  microblaze_register_exception_handler(XIL_EXCEPTION_ID_DIV_BY_ZERO, &DivByZeroException, NULL);
  microblaze_register_exception_handler(XIL_EXCEPTION_ID_M_AXI_I_EXCEPTION, &IBusException, NULL);
  microblaze_register_exception_handler(XIL_EXCEPTION_ID_M_AXI_D_EXCEPTION, &DBusException, NULL);
  microblaze_register_exception_handler(XIL_EXCEPTION_ID_STACK_VIOLATION, &StackViolationException, NULL);
  microblaze_register_exception_handler(XIL_EXCEPTION_ID_ILLEGAL_OPCODE, &IllegalOpcodeException, NULL);
  //microblaze_register_exception_handler(XIL_EXCEPTION_ID_UNALIGNED_ACCESS, &UnalignedAccessException, NULL);
  InitialiseInterruptControllerAndTimer(& Timer, & InterruptController);
  microblaze_enable_exceptions();
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "[DONE]\r\n");

  num_links = if_enumerate_interfaces();

  /* check if the one-gbe interface (physical id 0) is present */
  if (IF_ID_PRESENT == check_interface_valid(0)){
    // GT 7/3/2016 DIS_SLEEP = '1' and ENA_PAUSE = '1'
    UpdateGBEPHYConfiguration();

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Waiting for 1GBE SGMII to come out of reset ");

    uTimeoutCounter = 20;
    do
    {
      uReadReg = ReadBoardRegister(C_RD_BRD_CTL_STAT_0_ADDR);
    }while(((uReadReg & 0x1) != 0x1) && (uTimeoutCounter != 0));

    /* if we haven't TIMED OUT then we're OK */
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "%s\r\n", uTimeoutCounter == 0 ? "[TIMED OUT]" : "[OK]");

  } else {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Skipping 1GBE phy update...\r\n");
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Mezzanine locations\r\n");
  InitialiseMezzanineLocations();

  //AdcInit(&AdcContext);       /* now handled within mezz obj context */
  //uQSFPInit(QSFPContext_hdl);    /* now handled within mezz obj context */

  /*
   * call the qsfp+ state machine 4 times - this will set up the qsfp mezzanine cards
   * early in the init process - before hmc. This should reduce the boot up time slightly
   */

  for (uIndex = 0; uIndex < 4; uIndex++){
        /* QSFPStateMachine(QSFPContext_hdl); */
        run_qsfp_mezz_mgmt();
        Delay(100000);  /* 100ms */
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Waiting for HMC(s) to complete init...\r\n");

  /*
   *  Register C_RD_MEZZANINE_STAT_1_ADDR contains the firmware status of the 4 Mezz slots.
   *  Each byte represents the status of a Mezzanine site and indicates what functionality
   *  has been compiled in in firmware.
   *
   *  31 - 24 | 23 - 16| 15  - 8| 7 -  0 |
   *  +-------+--------+--------+--------+
   *  | Mezz3 | Mezz 2 | Mezz 1 | Mezz 0 |
   *  +-------+--------+--------+--------+
   *                            /         \
   *                           /           \
   *      +-------------------+             +----------------------+
   *      |                                                        |
   *
   *       b7       b6       b5        b4        b3-b1    b0
   *      +--------+--------+---------+---------+--------+---------+
   * QSFP | IF3 EN | IF2 EN | IF1 EN  | IF0 EN  | ID=001 | Present |
   *      +--------+--------+---------+---------+--------+---------+
   * HMC  | unused | unused | POST OK | INIT OK | ID=010 | Present |
   *      +--------+--------+---------+---------+--------+---------+
   * ADC  | unused | unused | unused  | unused  | ID=011 | Present |
   *      +--------+--------+---------+---------+--------+---------+
   *
   *      bit position in each of the above bytes (Mezz3...Mezz0)
   */

#define HMC_INIT_TIMEOUT    100    /* x 100ms */
#define BYTE_MASK_HMC_INIT  0x35  /* b00110101 */
#define BYTE_MASK_HMC_PRESENT  5  /* bxxxx0101 - look for this bit signature to determine HMC presence */

  uHMCBitMask = 0;
  uHMCRunInit = 0;

  /* dynamically build the bit mask for the hmc cards which are present */
  uReadReg = ReadBoardRegister(C_RD_MEZZANINE_STAT_1_ADDR);
  for (uIndex = 0; uIndex < 4; uIndex++){
    //if ((uReadReg & (0xF << (uIndex*8))) == (BYTE_MASK_HMC_PRESENT << (uIndex*8))){
    if ((MezzHandle[uIndex]->m_type == MEZ_BOARD_TYPE_HMC_R1000_0005) && (MezzHandle[uIndex]->m_allow_init == 1)){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] MEZZ %d has an HMC present. Trying to initialise...\r\n", uIndex);
      uHMCBitMask = uHMCBitMask | (BYTE_MASK_HMC_INIT << (uIndex*8));
      uHMCRunInit++;
    }
  }

  if (uHMCRunInit > 0){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] bit mask 0x%08x\r\n", uHMCBitMask);

    /* set timeout to a default value in case eeprom read fails */
    uHMCTimeout = HMC_INIT_TIMEOUT;

    /* only do if we have hmc cards present i.e. bitmask != 0 */
    if (uHMCBitMask){
      /* read hmc retry timeout and max attempts stored in pg15 of DS2433 EEPROM on Motherboard */
      if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) == XST_SUCCESS){
        if (DS2433ReadMem(rom, 0, data, 3, 0xE4, 0x1, MB_ONE_WIRE_PORT) == XST_SUCCESS){
          /* overwrite default */
          uHMCTimeout = (data[0] & 0xff) | (data[1] << 8);
          /*
           * Now ensure that the hmc timeout is a reasonable value since this is
           * user data in eeprom. Let's say a reasonable range is 1-60s else leave
           * as default value.
           */
          uHMCTimeout = (uHMCTimeout <= 600 && uHMCTimeout >= 10) ? uHMCTimeout : HMC_INIT_TIMEOUT;
          /* set max number of attempts before giving up */
          uHMC_Max_ReconfigCount = data[2];
          /* a reasonable value for the max hmc retry count is any value larger
           * than 3. This is to prevent it being set to zero when this feature is
           * first run without the correct config in flash */
          uHMC_Max_ReconfigCount = (uHMC_Max_ReconfigCount >= HMC_MAX_RECONFIG_COUNT) ? uHMC_Max_ReconfigCount : HMC_MAX_RECONFIG_COUNT; 
        }
      }
    }

    uTimeoutCounter = uHMCTimeout;

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] setting hmc timeout to %d ms with %d max attempts\r\n", (u32) uTimeoutCounter * 100, uHMC_Max_ReconfigCount);

    /* Now wait for all HMC cards present to POST and INIT otherwise time-out */
    do
    {
      uReadReg = ReadBoardRegister(C_RD_MEZZANINE_STAT_1_ADDR);
      /* Pat the Microblaze watchdog since this loop may potentially be longer than a watchdog timeout */
      if ((uTimeoutCounter % 10) == 0){    /* every second */
        XWdtTb_RestartWdt(& WatchdogTimer);
      }
    }while(((uReadReg & uHMCBitMask) != uHMCBitMask) && (uTimeoutCounter != 0));

    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] Mezzanine status register: 0x%08x\r\n", ReadBoardRegister(C_RD_MEZZANINE_STAT_1_ADDR));

    /* if one or some or all of the HMC's don't init and post... */
    if ((uReadReg & uHMCBitMask) != uHMCBitMask){
      /*
       * Read the current stats of the HMS'c stored in Persistent Memory - these are
       * the stats stored since the last hard reset / power cycle.
       */
#if 0
      PersistentMemory_ReadByte(HMC_RECONFIG_TOTAL_COUNT_INDEX, &uHMCReconfigCount);
#else
      PersistentMemory_ReadByte(HMC_RECONFIG_TOTAL_COUNT_INDEX, &uHMC_Total_ReconfigCount);
      PersistentMemory_ReadByte(HMC_RECONFIG_HMC0_COUNT_INDEX, &(uHMC_ReconfigCount[0]));
      PersistentMemory_ReadByte(HMC_RECONFIG_HMC1_COUNT_INDEX, &(uHMC_ReconfigCount[1]));
      PersistentMemory_ReadByte(HMC_RECONFIG_HMC2_COUNT_INDEX, &(uHMC_ReconfigCount[2]));
      PersistentMemory_ReadByte(HMC_RECONFIG_HMC3_COUNT_INDEX, &(uHMC_ReconfigCount[3]));
#endif

      /* determine which ones did not init and post */
      for (uIndex = 0; uIndex < 4; uIndex++){
        if ((MezzHandle[uIndex]->m_type == MEZ_BOARD_TYPE_HMC_R1000_0005) && (MezzHandle[uIndex]->m_allow_init)){
          if (((uReadReg >> uIndex * 8) & BYTE_MASK_HMC_INIT) != BYTE_MASK_HMC_INIT){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] HMC-%d did not init/post!\r\n", uIndex);
            /* increment the relevant failing HMC's counter */
            uHMC_ReconfigCount[uIndex] = uHMC_ReconfigCount[uIndex] + 1;
          }
        }
      }

      if (PMemState != PMEM_RETURN_ERROR){
        if (uHMC_Total_ReconfigCount < (uHMC_Max_ReconfigCount - 1)){   /* do n-1 times */

          /* for each reconfigure, increment total counter */
          uHMC_Total_ReconfigCount = uHMC_Total_ReconfigCount + 1;

          /* write back all counters to persistent memory registers */
          PersistentMemory_WriteByte(HMC_RECONFIG_TOTAL_COUNT_INDEX, uHMC_Total_ReconfigCount);
          PersistentMemory_WriteByte(HMC_RECONFIG_HMC0_COUNT_INDEX, uHMC_ReconfigCount[0]);
          PersistentMemory_WriteByte(HMC_RECONFIG_HMC1_COUNT_INDEX, uHMC_ReconfigCount[1]);
          PersistentMemory_WriteByte(HMC_RECONFIG_HMC2_COUNT_INDEX, uHMC_ReconfigCount[2]);
          PersistentMemory_WriteByte(HMC_RECONFIG_HMC3_COUNT_INDEX, uHMC_ReconfigCount[3]);

          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "HMC  [..] invoking reconfigure of FPGA\r\n");
          /* if currently a toolflow image - reboot from SDRAM else reboot from FLASH */
          if (((ReadBoardRegister(C_RD_VERSION_ADDR) & 0xff000000) >> 24) == 0){
            SetOutputMode(SDRAM_READ_MODE, 0x0); // Release flash bus when about to do a reboot
            ResetSdramReadAddress();
            AboutToBootFromSdram();
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "REBOOT - toolflow image: reconfigure from SDRAM\r\n");
          }
          Delay(100000);
          IcapeControllerInSystemReconfiguration();
        } else {    /* and do this the n-th time */
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "HMC  [..] maximum reconfiguration count reached [%d]\r\n", uHMC_Max_ReconfigCount);
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "HMC  [..] current hmc reconfig stats - mezz0:%d | mezz1:%d | mezz2:%d | mezz3:%d\r\n",
              uHMC_ReconfigCount[0], uHMC_ReconfigCount[1], uHMC_ReconfigCount[2], uHMC_ReconfigCount[3] );

          /*
           * The stats of each HMC will be recorded in the one-wire eeprom on the
           * HMC mezzanine card. It will be located at the start of page 15 of the
           * eeprom. Two values (32-bit unsigned) will be recorded, these are:
           * 1) the number of times the HMC card did not init/post
           * 2) the total number of maxed-out retries
           * These values will only be recorded upon the HMC retry mechanism
           * max'ing-out.
           *
           *  0x1E0 0x1E1 0x1E2 0x1E3
           *  byte0 byte1 byte2 byte3 of retries
           *
           *  0x1E4 0x1E5 0x1E6 0x1E7
           *  byte0 byte1 byte2 byte3 of total
           */

          u16 value[8];
          u32 retries, total;

          static const u16 one_wire_port_lookup[4] = {
            [0] = MEZ_0_ONE_WIRE_PORT,
            [1] = MEZ_1_ONE_WIRE_PORT,
            [2] = MEZ_2_ONE_WIRE_PORT,
            [3] = MEZ_3_ONE_WIRE_PORT };

          for (uIndex = 0; uIndex < 4; uIndex++){
            /* if this is an hmc card... */
            if ((MezzHandle[uIndex]->m_type == MEZ_BOARD_TYPE_HMC_R1000_0005) && (MezzHandle[uIndex]->m_allow_init == 1)){
              if (OneWireReadRom(rom, one_wire_port_lookup[uIndex]) == XST_SUCCESS){
                /* read from eeprom on respective hmc card */
                if (DS2433ReadMem(rom, 0, value, 8, 0xE0, 0x1, one_wire_port_lookup[uIndex]) == XST_SUCCESS){
                  /* increment the hmc stats */
                  retries = value[0] + (value[1] << 8) + (value[2] << 16) + (value[3] << 24) + uHMC_ReconfigCount[uIndex] ;
                  total = value[4] + (value[5] << 8) + (value[6] << 16) + (value[7] << 24) + uHMC_Max_ReconfigCount;
                  value[0] = retries & 0xff;
                  value[1] = (retries >> 8) & 0xff;
                  value[2] = (retries >> 16) & 0xff;
                  value[3] = (retries >> 24) & 0xff;
                  value[4] = total & 0xff;
                  value[5] = (total >> 8) & 0xff;
                  value[6] = (total >> 16) & 0xff;
                  value[7] = (total >> 24) & 0xff;
                  /* write back to eeprom on the respective hmc card */
                  if (DS2433WriteMem(rom, 0, value, 8, 0xE0, 0x1, one_wire_port_lookup[uIndex]) == XST_SUCCESS){
                    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [%02x] stored hmc stats in flash: retries=%d | total=%d\r\n", uIndex, retries, total);
                  } else {
                    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [%02x] error writing to DS2433\r\n", uIndex);
                  }
                } else {
                  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [%02x] error reading from DS2433\r\n", uIndex);
                }
              } else {
                log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [%02x] error reading ROM from DS2433\r\n", uIndex);
              }
            }
          }

        }
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "HMC  [..] error accessing persistent memory register\r\n");
      }
    } else {
      /* if we haven't TIMED OUT then we're OK */
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] HMCs initialized within %d ms[OK]\r\n", (u32) ((uHMCTimeout - uTimeoutCounter) * 100));
    }
  } else {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "HMC  [..] zero HMCs to initialize\r\n");
  }

  /*
   * clear the persistent memory register here so that the reconfig cycle starts
   * fresh on the next reset. Otherwise the only way to clear it would be via
   * hard reset or explicitly writing to it via casperfpga (fan_ctrl/i2c commands)
   */
  PersistentMemory_WriteByte(HMC_RECONFIG_TOTAL_COUNT_INDEX, 0);
  PersistentMemory_WriteByte(HMC_RECONFIG_HMC0_COUNT_INDEX, 0);
  PersistentMemory_WriteByte(HMC_RECONFIG_HMC1_COUNT_INDEX, 0);
  PersistentMemory_WriteByte(HMC_RECONFIG_HMC2_COUNT_INDEX, 0);
  PersistentMemory_WriteByte(HMC_RECONFIG_HMC3_COUNT_INDEX, 0);

#if defined(LINK_MON_RX_40GBE) || defined(RECONFIG_UPON_NO_DHCP)
  u16 timeout;

  /* read link/dhcp monitor timeout stored in pg15 of DS2433 EEPROM on Motherboard */
  if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) == XST_SUCCESS){
    if (DS2433ReadMem(rom, 0, data, 2, 0xE7, 0x1, MB_ONE_WIRE_PORT) == XST_SUCCESS){
      /* overwrite default */
      timeout = (data[0] & 0xff) | (data[1] << 8);
      /*
       * The link/dhcp timeout value is a user set parameter in eeprom. We
       * therefore have to ensure that it is always set to a sane value which is
       * determined by the amount time a switch can potentially take to set up
       * the link. To ensure that we do not get ourselves into a link-reset loop
       * where we do not allow the switch sufficient time to set up the link, we
       * will progressively increment this timeout value based on the previous
       * number of link/dhcp timeouts.
       * Now ensure that the link timeout is a reasonable (minimum) value since this is user data in eeprom and the
       * switch may take some time to initialize the link before we see activity, else leave as default value.
       */
#ifdef LINK_MON_RX_40GBE
      uLinkTimeoutMax = (timeout >= LINK_MON_COUNTER_MIN_VALUE) ? timeout : LINK_MON_COUNTER_DEFAULT_VALUE;
#elif defined(RECONFIG_UPON_NO_DHCP)

      if (PMemState != PMEM_RETURN_ERROR){
        ret = PersistentMemory_ReadByte(DHCP_RECONFIG_COUNT_INDEX, &uDHCPReconfigCount);
      }

      if ((PMemState == PMEM_RETURN_ERROR) || (ret != PMEM_RETURN_OK)){
        uDHCPReconfigCount = 25; /* ensure that this var is always a sane value */
      }

      uDHCPTimeoutMax = (timeout >= DHCP_MON_COUNTER_MIN_VALUE) ? timeout : DHCP_MON_COUNTER_DEFAULT_VALUE;
      uDHCPTimeoutMax = uDHCPTimeoutMax + (uDHCPReconfigCount * 10);   /* times 10 to convert to seconds */

      /* cache the max dhcp timeout value in order for eth cmd to send later
       * TODO: wrap in module functions and remove global scope
       */
      GlobalDHCPMonitorTimeout = uDHCPTimeoutMax;
#endif
    }
  }
#ifdef LINK_MON_RX_40GBE
  const char *const s = "40gbe-link-mon";
  timeout = uLinkTimeoutMax;
#else
  const char *const s = "no-dhcp-mon";
  timeout = uDHCPTimeoutMax;
#endif

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] setting %s timeout to %d ms\r\n", s, (u32) timeout * 100);
#endif

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] Interface parameters\r\n");
  InitialiseEthernetInterfaceParameters();

#if 0
  /* set bit#1 in C_WR_BRD_CTL_STAT_1_ADDR to 1 to connect 40gbe to user fabric */
  WriteBoardRegister(C_WR_BRD_CTL_STAT_1_ADDR, 0x2);
#endif

  iStatus = XST_SUCCESS;

  ReadAndPrintFPGADNA();
  ReadAndPrintPeralexSerial();

  /*
   * read dhcp init and retry times stored in pg15 of DS2433 EEPROM on Motherboard/
   * -> Init wait time at addr 0x1E0(LSB) and 0x1E1(MSB)
   * -> Retry time at addr 0x1E2(LSB) and 0x1E3(MSB)
   */

  if (OneWireReadRom(rom, MB_ONE_WIRE_PORT) == XST_SUCCESS){
    if (DS2433ReadMem(rom, 0, data, 4, 0xE0, 0x1, MB_ONE_WIRE_PORT) == XST_SUCCESS){
      dhcp_wait = (data[0] & 0xff) | (data[1] << 8) ;
      dhcp_retry = (data[2] & 0xff) | (data[3] << 8) ;
      dhcp_set = 1;
    }
  } /* else run with the default values automatically set when interface initialized */


  //for (uPhysicalEthernetId = 0; uPhysicalEthernetId < NUM_ETHERNET_INTERFACES; uPhysicalEthernetId++){
  for (logical_link = 0; logical_link < num_links; logical_link++){
    uPhysicalEthernetId = get_physical_interface_id(logical_link);       /* returns the physical interface for a logical link id */

    u8 *mac_addr = if_generate_mac_addr_array(uPhysicalEthernetId);

    pIFObjectPtr[uPhysicalEthernetId] = InterfaceInit(uPhysicalEthernetId,
        (u8 *) uReceiveBuffer,
        (RX_BUFFER_MAX * 4),
        (u8 *) uTransmitBuffer,
        (TX_BUFFER_MAX * 4),
        mac_addr);

    if (NULL == pIFObjectPtr[uPhysicalEthernetId]){
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_FATAL, "IF   [%02x] Init error!\r\n", uPhysicalEthernetId);
    } else {

      u8 *hostname = if_generate_hostname_string(uPhysicalEthernetId);

      eventDHCPOnMsgBuilt(pIFObjectPtr[uPhysicalEthernetId], &vSendDHCPMsg, NULL);
      eventDHCPOnLeaseAcqd(pIFObjectPtr[uPhysicalEthernetId], &vSetInterfaceConfig, NULL);

      vDHCPSetHostName(pIFObjectPtr[uPhysicalEthernetId], (char *) hostname);

      /* the 1gbe link exhibits an UP - DOWN - UP behaviour most of the time.
         We thus want to delay the start of dhcp till these transitions have settled.
         The delay is determined by the DHCP_SM_WAIT macro. */
      //if (uPhysicalEthernetId == 0){  /* comment out since we now want it on all links */
      uDHCPSetWaitOnInitFlag(pIFObjectPtr[uPhysicalEthernetId]);
      if (dhcp_set){
        uDHCPSetRetryInterval(pIFObjectPtr[uPhysicalEthernetId], (u32) dhcp_retry);
        uDHCPSetInitWait(pIFObjectPtr[uPhysicalEthernetId], (u32) dhcp_wait);
      }
      //}

      /*
       * NOTE: specifically for site - try to request the previous ip from the
       * server in an attempt to reduce dhcp traffic at init. On site, only one
       * 40gbe link is connected (i/f 1) and provision is currently only made
       * for this one ip to persist between reconfig's.
       * TODO: perhaps expand for all links - will require more persistent memory
       */
      if (uPhysicalEthernetId == 1){
        /* FIXME: move declarartions up */
        u8 byte;
        u32 tempIP = 0;

        /* check if we have cached the ip from the previous reconfigure cycle. */
        PersistentMemory_ReadByte(DHCP_CACHED_IP_STATE_INDEX, &byte);
        if (byte == 1){
          /* set the IP to cached IP */
          PersistentMemory_ReadByte(DHCP_CACHED_IP_OCT0_INDEX, &byte);
          tempIP = tempIP | ((u32) byte << 24);
          PersistentMemory_ReadByte(DHCP_CACHED_IP_OCT1_INDEX, &byte);
          tempIP = tempIP | ((u32) byte << 16);
          PersistentMemory_ReadByte(DHCP_CACHED_IP_OCT2_INDEX, &byte);
          tempIP = tempIP | ((u32) byte << 8);
          PersistentMemory_ReadByte(DHCP_CACHED_IP_OCT3_INDEX, &byte);
          tempIP = tempIP | ((u32) byte);

          /* pre-load state machine to attempt to request the same lease */
          uDHCPSetRequestCachedIP(pIFObjectPtr[uPhysicalEthernetId], tempIP);
          /*
           * Some more site-specific stuff:
           * Also set the fabric interface parameters "preemptively" with the
           * knowledge that we will always receive the same dhcp ip lease for the
           * same switch interface on site. These settings only persist between
           * "warm" reboots and thus will not have an impact after power-down.
           * FIXME: add functionality to clear fabric interface parameters upon
           * DHCP NAK receipt - this will add another safety check that the
           * interface was not somehow configured with incorrect lease parameters.
           * Background: this feature added to help overcome the DHCP "bottleneck"
           * issue currently experienced on site. This allows us to communicate
           * with the skarab before DHCP actually completes. Use of this feature
           * together with the RECONFIG_UPON_NO_DHCP feature is strongly
           * discouraged.
           */
#ifdef PREEMPT_CONFIGURE_FABRIC_IF
          u32 tempMask = 0;

          SetFabricSourceIPAddress(uPhysicalEthernetId, tempIP);
          ProgramARPCacheEntry(uPhysicalEthernetId, byte,
              uEthernetFabricMacHigh[uPhysicalEthernetId],
              ((uEthernetFabricMacMid[uPhysicalEthernetId] << 16) |
               uEthernetFabricMacLow[uPhysicalEthernetId]));
          //uEthernetFabricIPAddress[uPhysicalEthernetId] = tempIP;

          PersistentMemory_ReadByte(DHCP_CACHED_MASK_OCT0_INDEX, &byte);
          tempMask = tempMask | ((u32) byte << 24);
          PersistentMemory_ReadByte(DHCP_CACHED_MASK_OCT1_INDEX, &byte);
          tempMask = tempMask | ((u32) byte << 16);
          PersistentMemory_ReadByte(DHCP_CACHED_MASK_OCT2_INDEX, &byte);
          tempMask = tempMask | ((u32) byte << 8);
          PersistentMemory_ReadByte(DHCP_CACHED_MASK_OCT3_INDEX, &byte);
          tempMask = tempMask | ((u32) byte);

          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IF   [%02x] setting cached netmask %x\r\n", uPhysicalEthernetId, tempMask);
          SetFabricNetmask(uPhysicalEthernetId, tempMask);

          PersistentMemory_ReadByte(DHCP_CACHED_GW_OCT3_INDEX, &byte);

          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "IF   [%02x] setting cached gateway %x\r\n", uPhysicalEthernetId, byte);
          SetFabricGatewayARPCacheAddress(uPhysicalEthernetId, byte);

          //uEthernetSubnet[uPhysicalEthernetId] = (tempIP & tempMask);   /* TODO: global */
          pIFObjectPtr[uPhysicalEthernetId]->uIFEnableArpRequests = ARP_REQUESTS_ENABLE;

          IFConfig(pIFObjectPtr[uPhysicalEthernetId], tempIP, tempMask);

          /* legacy dhcp states */
          uDHCPState[uPhysicalEthernetId] = DHCP_STATE_COMPLETE;
          EnableFabricInterface(uPhysicalEthernetId, 1);
#endif
        }
      }
    }
  }

  iStatus = log_time_sync_devices();
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "TIME [..] {%s} sync monitoring run time counters\r\n", XST_SUCCESS == iStatus ? "OK" : "FAIL");

  u8 aux_flags = 0;
  PersistentMemory_ReadByte(AUX_SKARAB_FLAGS_INDEX, &aux_flags);

  if ((aux_flags & 0x2) == 0){  /* if write protection bit is not set - then this code section can write to it */
    if (uReg & 0x40){
      aux_flags = aux_flags | 0x3;   /* set the location bit for sdram and the protection bit*/
      xil_printf("set sdram prev loc %d\r\n", aux_flags);
    } else {
      aux_flags = aux_flags & 0xE;   /* clear the bit - booted from flash */
      aux_flags = aux_flags | 0x2;   /* and set the protection bit */
      xil_printf("set flash prev loc\r\n");
    }
  } /* else do nothing - this bringup was probably caused by
       a reset of some sort (register, wdt, etc.) and in this
       case the FLASH_SDRAM_CONFIG_IO_REG_ADDRESS register
       will not truly reflect where the previous reconfigure
       was from.
     */
  PersistentMemory_WriteByte(AUX_SKARAB_FLAGS_INDEX, aux_flags);
#if 0
  xil_printf("aux_flags = %d\r\n", aux_flags);
  PersistentMemory_ReadByte(AUX_SKARAB_FLAGS_INDEX, &aux_flags);
  xil_printf("aux_flags = %d\r\n", aux_flags);
#endif

  //WriteBoardRegister(C_WR_FRONT_PANEL_STAT_LED_ADDR, 255);
  while(1)
  {

    /* these mezz mgmt functions are split since the flags guarding them are set
     * by other functions and cmds independently e.g. uQSFPUpdateStatusEnable is set in
     * QSFPResetAndProgramCommandHandler()
     */
    if (uQSFPUpdateStatusEnable == UPDATE_QSFP_STATUS){
      uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;
      run_qsfp_mezz_mgmt();
    }

    if (uADC32RF45X2UpdateStatusEnable == UPDATE_ADC32RF45X2_STATUS){
      uADC32RF45X2UpdateStatusEnable = DO_NOT_UPDATE_ADC32RF45X2_STATUS;
      run_adc_mezz_mgmt();
    }


#if 0
    if (uQSFPMezzaninePresent == QSFP_MEZZANINE_PRESENT){
      //UpdateQSFPStatus();
      if (uQSFPUpdateStatusEnable == UPDATE_QSFP_STATUS){
        uQSFPUpdateStatusEnable = DO_NOT_UPDATE_QSFP_STATUS;
        QSFPStateMachine(QSFPContext_hdl);
      }
    }

    if (uADC32RF45X2MezzaninePresent == ADC32RF45X2_MEZZANINE_PRESENT){
      if (uADC32RF45X2UpdateStatusEnable == UPDATE_ADC32RF45X2_STATUS){
        uADC32RF45X2UpdateStatusEnable = DO_NOT_UPDATE_ADC32RF45X2_STATUS;
        AdcStateMachine(&AdcContext);
      }
    }
#endif


    //for (uPhysicalEthernetId = 0; uPhysicalEthernetId < NUM_ETHERNET_INTERFACES; uPhysicalEthernetId++){

    for (logical_link = 0; logical_link < num_links; logical_link++){
      uPhysicalEthernetId = get_physical_interface_id(logical_link);

      UpdateEthernetLinkUpStatus(pIFObjectPtr[uPhysicalEthernetId]);

      if (pIFObjectPtr[uPhysicalEthernetId]->uIFLinkStatus == LINK_UP)
      {
        uNumWords = GetHostReceiveBufferLevel(uPhysicalEthernetId);
        if (uNumWords != 0)
        {
          /* zero the receive buffer */
          memset((u8 *) uReceiveBuffer, 0, RX_BUFFER_MAX * 4);

          /* General packet reception handling:
             FILTER ( by layer and packet type)
             V
             VERIFY (Checksum on relevant layers)
             V
             VALIDATE (Protocol specifications)
             V
             PROCESS  (Do the work) */

          // Read packet into receive buffer
          iStatus  = ReadHostPacket(uPhysicalEthernetId, uReceiveBuffer, uNumWords);
          if (iStatus != XST_SUCCESS){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Read Host Packet Error!\r\n");
          } else {
#ifdef TIME_PROFILE
            time1 = XWdtTb_ReadReg(WatchdogTimerConfig->BaseAddr, XWT_TWCSR0_OFFSET);
#endif
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "Read %d words in host packet!\r\n", uNumWords);
            pBuffer = (u16*) uReceiveBuffer;

            /* convert the number of 32bit words to a number of 16bit half-words */
            pIFObjectPtr[uPhysicalEthernetId]->uNumWordsRead = uNumWords;
            uSize = uNumWords << 1;

            /*
             * deep packet inspection - this was added to specifically improve programming performance by bypassing the
             * subsequent half-word endian swapping since this is not needed for the control packet parsing code
             */
            if (pBuffer[18] != UDP_CONTROL_PORT){
              /* correct the endianess */
              for (uIndex = 0; uIndex < uSize; uIndex++){
                //pBuffer[uIndex] = Xil_EndianSwap16(pBuffer[uIndex]);
#if 0
                pBuffer[uIndex] = (u16) (((pBuffer[uIndex] & 0xFF00U) >> 8U) | ((pBuffer[uIndex] & 0x00FFU) << 8U));
#else
                buf = (unsigned char *) (pBuffer + uIndex);
                temp = buf[0];
                buf[0] = buf[1];
                buf[1] = temp;
                //pBuffer++;
                /* TODO: check this logic again!! */
#endif
              }

              /* reset pBuffer to point to start of buffer since we altered it */
              //pBuffer = (u16*) &(uReceiveBuffer[uPhysicalEthernetId][0]);

              uPacketType = uRecvPacketFilter(pIFObjectPtr[uPhysicalEthernetId]);
            } else {
              uPacketType = PACKET_FILTER_CONTROL;
            }

            log_printf(LOG_SELECT_IFACE, LOG_LEVEL_DEBUG, "PCKT [%02x] Received packet type %d\r\n", uPhysicalEthernetId, uPacketType);

            /* do relevant checksum validation */
            switch(uPacketType){
              case PACKET_FILTER_DHCP:
              case PACKET_FILTER_CONTROL:
                /* verify udp checksum */
                if (uUDPChecksumCalc((u8 *) pBuffer, &uChecksum) == 0){
                  if(uChecksum != 0xffff){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_ERROR, "%s [%02x] RX - Invalid UDP Checksum!\r\n", uPacketType == PACKET_FILTER_DHCP ? "DHCP" : "CTRL", uPhysicalEthernetId);
#if 0
                    for (u8 n = 0; n < 50; n++){
                      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "%04x ", (pBuffer[n]));
                    }
                    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "\r\n");
#endif
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_UDP_CHK_ERR);
                    break;
                  }
                }
                /* else no break statement - fall through */

              case PACKET_FILTER_ICMP:
                /* ip layer checks */
                /* FIXME: TODO check ip destination address */
                /* verify ip header checksum */
                if(uIPChecksumCalc((u8 *) pBuffer, &uChecksum) == 0){
                  if(uChecksum != 0xffff){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_ERROR, "%s [%02x] RX - Invalid IP Checksum!\r\n", uPacketType == PACKET_FILTER_DHCP ? "DHCP" : uPacketType == PACKET_FILTER_ICMP ? "ICMP" : "CTRL", uPhysicalEthernetId);
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_IP_CHK_ERR);
                    break;
                  }
                }
                /* else no break statement - fall through */

              case PACKET_FILTER_ARP:
                if_valid_rx_set(uPhysicalEthernetId);
                uValidate = 1;
                break;

              case PACKET_FILTER_IGMP_UNHANDLED:
              case PACKET_FILTER_PIM_UNHANDLED:
              case PACKET_FILTER_TCP_UNHANDLED:
              case PACKET_FILTER_LLDP_UNHANDLED:
                /* disable the dhcp unbound monitor loop upon receipt of any valid known packet */
                if_valid_rx_set(uPhysicalEthernetId);

              case PACKET_FILTER_UNKNOWN:
              case PACKET_FILTER_UNKNOWN_ETH:
              case PACKET_FILTER_UNKNOWN_IP:
              case PACKET_FILTER_UNKNOWN_UDP:
              case PACKET_FILTER_ERROR:
                log_printf(LOG_SELECT_IFACE, LOG_LEVEL_DEBUG, "PCKT [%02x] packet filter: %s\r\n", uPhysicalEthernetId, uPacketType == PACKET_FILTER_ERROR ? "error" : "unhandled");
              case PACKET_FILTER_DROP:
              default:
                /* do nothing */
                uValidate = 0;
                //uDump = 1;
                break;
            }

            /* do further protocol specific validation */
            if (uValidate){
              switch(uPacketType){
                case PACKET_FILTER_DHCP:
                  if (uDHCPMessageValidate(pIFObjectPtr[uPhysicalEthernetId]) == DHCP_RETURN_OK){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "DHCP [%02x] valid packet received!\r\n", uPhysicalEthernetId);
                    uDHCPSetGotMsgFlag(pIFObjectPtr[uPhysicalEthernetId]);
                    uDHCPStateMachine(pIFObjectPtr[uPhysicalEthernetId]);   /* run the DHCP state machine immediately */
                  } else {
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_DHCP_INVALID);
                    /* quiet the debug output here a bit since there will be lots of dhcp bcast traffic at startup */
                    /* Note: dhcp server bcast replies will show up as invalid dhcp packets as well */
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_TRACE, "DHCP [%02x] invalid packet received!\r\n", uPhysicalEthernetId);
                  }
                  break;

                case PACKET_FILTER_CONTROL:
                  /* the following printf statement should have trace print level in order to prevent performance hit during programming 
                     since printing out to the serial port adds quite a bit of overhead and therefore time */
                  log_printf(LOG_SELECT_IFACE, LOG_LEVEL_TRACE, "CTRL [%02x] valid packet received!\r\n", uPhysicalEthernetId);
                  /* TODO: validate - for now, hand over to Peralex code */
                  uFlagRunTask_CTRL[uPhysicalEthernetId] = 1;
                  break;

                case PACKET_FILTER_ICMP:
                  if (uICMPMessageValidate(pIFObjectPtr[uPhysicalEthernetId]) == ICMP_RETURN_OK){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "ICMP [%02x] valid packet received!\r\n", uPhysicalEthernetId);
                    uFlagRunTask_ICMP_Reply[uPhysicalEthernetId] = 1;
                  } else {
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_ICMP_INVALID);
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "ICMP [%02x] invalid packet received!\r\n", uPhysicalEthernetId);
                  }
                  break;

                case PACKET_FILTER_ARP:
                  iStatus =  uARPMessageValidateReply(pIFObjectPtr[uPhysicalEthernetId]);
                  if (iStatus == ARP_RETURN_OFF){
                    asm("nop");
                  } else if (iStatus == ARP_RETURN_REPLY){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_TRACE, "ARP  [%02x] valid reply received!\r\n", uPhysicalEthernetId);
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_ARP_REPLY);
                    uFlagRunTask_ARP_Process[uPhysicalEthernetId] = 1;
                  } else if (iStatus == ARP_RETURN_REQUEST){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_TRACE, "ARP  [%02x] valid request received!\r\n", uPhysicalEthernetId);
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_ARP_REQUEST);
                    uFlagRunTask_ARP_Respond[uPhysicalEthernetId] = 1;
                  } else if (iStatus == ARP_RETURN_CONFLICT){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_ERROR, "ARP  [%02x] network address conflict!\r\n", uPhysicalEthernetId);
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_ARP_CONFLICT);
                    vDHCPStateMachineReset(pIFObjectPtr[uPhysicalEthernetId]);
                    uDHCPSetStateMachineEnable(pIFObjectPtr[uPhysicalEthernetId], SM_TRUE);
                    //uDHCPStateMachine(&DHCPContextState[uPhysicalEthernetId]);   /* run the DHCP state machine immediately */
                    uFlagRunTask_DHCP[uPhysicalEthernetId] = 1;
                  } else if (iStatus == ARP_RETURN_INVALID){
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "ARP  [%02x] malformed packet!\r\n", uPhysicalEthernetId);
                    IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], RX_ARP_INVALID);
                  } else {
                    /* arp packet probably not meant for us */
                    log_printf(LOG_SELECT_IFACE, LOG_LEVEL_TRACE, "ARP  [%02x] dropping packet!\r\n", uPhysicalEthernetId);
                  }
                  break;

                case PACKET_FILTER_IGMP_UNHANDLED:
                case PACKET_FILTER_PIM_UNHANDLED:
                case PACKET_FILTER_TCP_UNHANDLED:
                case PACKET_FILTER_LLDP_UNHANDLED:
                case PACKET_FILTER_UNKNOWN:
                case PACKET_FILTER_UNKNOWN_ETH:
                case PACKET_FILTER_UNKNOWN_IP:
                case PACKET_FILTER_UNKNOWN_UDP:
                  break;

                case PACKET_FILTER_ERROR:
                  break;

                case PACKET_FILTER_DROP:
                default:
                  break;
              }
              uValidate = 0;
            }
          }
        }
      }

      /* simple interrupt driven multi-tasking scheduler */

      //----------------------------------------------------------------------------//
      //  DHCP TASK                                                                 //
      //  Triggered on timer interrupt and DHCP message receipt                     //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_DHCP[uPhysicalEthernetId]){
        uFlagRunTask_DHCP[uPhysicalEthernetId] = 0;     /* reset task flag */
        uDHCPStateMachine(pIFObjectPtr[uPhysicalEthernetId]);
      }

      //----------------------------------------------------------------------------//
      //  IGMP TASK                                                                 //
      //  Triggered on timer interrupt and IGMP control command                     //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_IGMP[uPhysicalEthernetId]){
        uFlagRunTask_IGMP[uPhysicalEthernetId] = 0;     /* reset task flag */
        if (pIFObjectPtr[uPhysicalEthernetId]->uIFLinkStatus == LINK_UP){
          uIGMPStateMachine(uPhysicalEthernetId);
        }
      }

      //----------------------------------------------------------------------------//
      //  CONTROL TASK                                                              //
      //  Triggered on CONTROL message receipt                                      //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_CTRL[uPhysicalEthernetId]){
        log_printf(LOG_SELECT_CTRL, LOG_LEVEL_TRACE, "CTRL [%02x] Running control task...\r\n", uPhysicalEthernetId);
        /* TODO: for now, run Peralex Control Protocol handler, slightly hackish */
        /* flip the endianess again */
        pBuffer = (u16 *) pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr;
        // pBuffer = (u16*) &(uReceiveBuffer[uPhysicalEthernetId][0]);

        /* convert the number of 32bit words to a number of 16bit half-words */
        uSize = pIFObjectPtr[uPhysicalEthernetId]->uNumWordsRead << 1;

        iStatus = EthernetRecvHandler(uPhysicalEthernetId, pIFObjectPtr[uPhysicalEthernetId]->uNumWordsRead, &uResponsePacketLength);

        if (iStatus == XST_SUCCESS){
          // Send the response packet now
#ifdef TIME_PROFILE
          time2 = XWdtTb_ReadReg(WatchdogTimerConfig->BaseAddr, XWT_TWCSR0_OFFSET);
#endif
          uResponsePacketLength = (uResponsePacketLength >> 2);
          iStatus = TransmitHostPacket(uPhysicalEthernetId, & uTransmitBuffer[0], uResponsePacketLength);
          if (iStatus != XST_SUCCESS){
            log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [%02x] Unable to send response\r\n", uPhysicalEthernetId);
          } else {
            IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_UDP_CTRL_OK);
          }
#ifdef TIME_PROFILE
          log_printf(LOG_SELECT_CTRL, LOG_LEVEL_TRACE, "time2 %08x - time1 %08x = %08x\r\n", time2, time1, time2 - time1);
#endif
        }

        uFlagRunTask_CTRL[uPhysicalEthernetId] = 0;
      }


      //----------------------------------------------------------------------------//
      //  ICMP TASK                                                                 //
      //  Triggered on ICMP message receipt                                         //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_ICMP_Reply[uPhysicalEthernetId]){
        iStatus = uICMPBuildReplyMessage(pIFObjectPtr[uPhysicalEthernetId]);
        if (iStatus != ICMP_RETURN_OK){
          log_printf(LOG_SELECT_ICMP, LOG_LEVEL_INFO, "ICMP [%02x] Packet build error!\r\n", uPhysicalEthernetId);
        } else {
          /* TODO: move this code down one layer of abstraction - similar to DHCP API  */
          uSize = (u32) pIFObjectPtr[uPhysicalEthernetId]->uMsgSize;    /* bytes */
          pBuffer = (u16 *) pIFObjectPtr[uPhysicalEthernetId]->pUserTxBufferPtr;
          if (pBuffer != NULL){
            uSize = uSize >> 1;     /* convert number of bytes to number of 16bit half-words */
            for (uIndex = 0; uIndex < uSize; uIndex++){
              pBuffer[uIndex] = Xil_EndianSwap16(pBuffer[uIndex]);
            }
            uSize = uSize >> 1;   /*  now convert quantity to amount of 32-bit words */
            if (TransmitHostPacket(uPhysicalEthernetId, (u32 *) pBuffer, uSize) != XST_SUCCESS){
              log_printf(LOG_SELECT_ICMP, LOG_LEVEL_ERROR, "ICMP [%02x] unable to send reply\r\n", 1);
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_IP_ICMP_REPLY_ERR);
            } else {
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_IP_ICMP_REPLY_OK);
            }
          }
        }
        uFlagRunTask_ICMP_Reply[uPhysicalEthernetId] = 0;
        uFlagRunTask_Diagnostics = 1;   /* dump interface counters to console upon ping */
      }


      //----------------------------------------------------------------------------//
      //  ARP PROCESSING TASK                                                       //
      //  Triggered on ARP reply message reception                                  //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_ARP_Process[uPhysicalEthernetId]){

        /* TODO FIXME refactor following lines: remove inline declarations, etc. */
        u32 ip;
        ip = (pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_PROTO_ADDR_OFFSET + 3] & 0xff);

        u32 u = ((pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET] << 8) & 0xff00) |
          (pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET + 1] & 0xff); 
        u32 l = ((pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET + 2] << 24) & 0xff000000) |
          ((pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET + 3] << 16) & 0xff0000) |
          ((pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET + 4] << 8) & 0xff00) |
          (pIFObjectPtr[uPhysicalEthernetId]->pUserRxBufferPtr[ARP_FRAME_BASE + ARP_SRC_HW_ADDR_OFFSET + 5] & 0xff); 

        log_printf(LOG_SELECT_ARP, LOG_LEVEL_INFO, "ARP  [%02x] ENTRY - IP#: %03d MAC: %04x.%04x.%04x\r\n", uPhysicalEthernetId, ip, (u & 0xffff), ((l >> 16) & 0xffff), (l & 0xffff));
        /* TODO: API functions */
        ProgramARPCacheEntry(uPhysicalEthernetId, ip, u, l);

        uFlagRunTask_ARP_Process[uPhysicalEthernetId] = 0;
      }

      //----------------------------------------------------------------------------//
      //  ARP RESPOND TASK                                                          //
      //  Triggered on ARP request message reception                                //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_ARP_Respond[uPhysicalEthernetId]){
        iStatus = uARPBuildMessage(pIFObjectPtr[uPhysicalEthernetId], ARP_OPCODE_REPLY, 0);
        if (iStatus != ARP_RETURN_OK){
          log_printf(LOG_SELECT_ARP, LOG_LEVEL_ERROR, "ARP  [%02x] Packet build error!\r\n", uPhysicalEthernetId);
        } else {
          /* TODO: move this code down one layer of abstraction - similar to DHCP API  */
          uSize = (u32) pIFObjectPtr[uPhysicalEthernetId]->uMsgSize;    /* bytes */
          pBuffer = (u16 *) pIFObjectPtr[uPhysicalEthernetId]->pUserTxBufferPtr;
          if (pBuffer != NULL){
            uSize = uSize >> 1;     /* convert number of bytes to number of 16bit half-words */
            for (uIndex = 0; uIndex < uSize; uIndex++){
              pBuffer[uIndex] = Xil_EndianSwap16(pBuffer[uIndex]);
            }
            uSize = uSize >> 1;   /*  now convert quantity to amount of 32-bit words */
            if (TransmitHostPacket(uPhysicalEthernetId, (u32 *) pBuffer, uSize) != XST_SUCCESS){
              log_printf(LOG_SELECT_ARP, LOG_LEVEL_ERROR, "ARP  [%02x] unable to send reply\r\n", 1);
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_ETH_ARP_ERR);
            } else {
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_ETH_ARP_REPLY_OK);
            }
          }
        }
        uFlagRunTask_ARP_Respond[uPhysicalEthernetId] = 0;
      }

      //----------------------------------------------------------------------------//
      //  ARP REQUEST TASK                                                          //
      //----------------------------------------------------------------------------//
      if (uFlagRunTask_ARP_Requests[uPhysicalEthernetId] == ARP_REQUEST_UPDATE){
        uFlagRunTask_ARP_Requests[uPhysicalEthernetId] = ARP_REQUEST_DONT_UPDATE;
        ArpRequestHandler(pIFObjectPtr[uPhysicalEthernetId]);
      }

      //----------------------------------------------------------------------------//
      //  LLDP TASK                                                                 //
      //  Triggered on timer interrupt                                              //
      //----------------------------------------------------------------------------//
      if(uFlagRunTask_LLDP[uPhysicalEthernetId]){
        /* only send lldp packets if the link is up */
        if (pIFObjectPtr[uPhysicalEthernetId]->uIFLinkStatus == LINK_UP){
          iStatus = uLLDPBuildPacket(uPhysicalEthernetId, (u8*) uTransmitBuffer, &uResponsePacketLength);
          if(iStatus == XST_SUCCESS){
            uSize = uResponsePacketLength >> 1; /* 16 bit words */
            pBuffer = (u16 *) uTransmitBuffer;

            for(uIndex = 0; uIndex < uSize; uIndex++){
              pBuffer[uIndex] = Xil_EndianSwap16(pBuffer[uIndex]);
            }
            uSize = uSize >> 1; /* 32 bit words*/
            iStatus = TransmitHostPacket(uPhysicalEthernetId, (u32*) &pBuffer[0], uSize);
            if (iStatus != XST_SUCCESS){
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_ETH_LLDP_ERR);
            } else {
              IFCounterIncr(pIFObjectPtr[uPhysicalEthernetId], TX_ETH_LLDP_OK);
            }
          }
        }
        uFlagRunTask_LLDP[uPhysicalEthernetId] = 0;
      }

    }   /* end network for-loop */

    //----------------------------------------------------------------------------//
    //  40GBE RX LINK MONITOR TASK                                                //
    //  Triggered on timer interrupt                                              //
    //----------------------------------------------------------------------------//

    /*
     * This is another MeerKAT site-specific feature. This feature will
     * reconfigure the FGPA if the 40gbe link on interface id:1 is up but no
     * *rx* activity has been detected after n seconds. This is to
     * combat/eliminate possible rx link issues on site.
     * FIXME: rather than reconfiguring & rebooting - why not just bounce the link? (TODO)
     */
#if 0
#if defined(LINK_MON_RX_40GBE) && (NUM_ETHERNET_INTERFACES > 1)
    if (uFlagRunTask_LinkWatchdog && (pIFObjectPtr[1]->uIFLinkStatus == LINK_UP)) {
      uFlagRunTask_LinkWatchdog = 0;

      /* if the rx side of the link is active, stop the timer by setting timeout
       * to zero */
      if (pIFObjectPtr[1]->uIFLinkRxActive == 1){
        LinkTimeout = 0;
      }

      /*
       * If 40gbe link one is not up yet, increment timeout counter.  However,
       * once the link comes up, this should never-ever run again since
       * LinkTimeout is set to 0 and can never enter this conditional to alter
       * it. This also relies on LinkTimeout not equalling 0 at startup - see
       * initialisation of LinkTimeout at start of main().
       */

      /* FIXME TODO: we may actually want the link to be monitored constantly since we've been experiencing "link
       * failures" on site... */

      if (LinkTimeout != 0){
        LinkTimeout++;
      }

      /* timeout after n seconds */
      if (LinkTimeout > uLinkTimeoutMax){
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "REBOOT - 40gbe rx link-1 inactive for %dms!\r\n", uLinkTimeoutMax * 100);
        if (((ReadBoardRegister(C_RD_VERSION_ADDR) & 0xff000000) >> 24) == 0){
          /* if it's a toolflow image */
          SetOutputMode(SDRAM_READ_MODE, 0x0); /* Release flash bus when about to reboot */
          ResetSdramReadAddress();
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "REBOOT - toolflow image: reconfigure from SDRAM\r\n");
          AboutToBootFromSdram();
          /* TODO: wouldn't it be better/safer to just reset the 40gbe link via the register? */
        }
        uDoReboot = REBOOT_REQUESTED;
      }
    }

#endif
#endif

    //----------------------------------------------------------------------------//
    //  CHECK DHCP BOUND TASK                                                     //
    //  Triggered on timer interrupt                                              //
    //----------------------------------------------------------------------------//
    /*
     * keep track of how long we have been "unbound" w.r.t. dhcp - this could be
     * an indication that the link did not come up cleanly and the only way to
     * fix this is through a reset. Note: this loop stops upon receipt of a valid
     * known packet
     */

#ifdef RECONFIG_UPON_NO_DHCP
#if 0
    //if((uFlagRunTask_CheckDHCPBound) && (TRUE != uValidPacketRx)){
    if((uFlagRunTask_CheckDHCPBound)){
      uFlagRunTask_CheckDHCPBound = 0;
      uDHCPBoundTimeout = 0;
      //for(uPhysicalEthernetId = 0; uPhysicalEthernetId < NUM_ETHERNET_INTERFACES; uPhysicalEthernetId++){
      for (logical_link = 0; logical_link < num_links; logical_link++){
        uPhysicalEthernetId = get_physical_interface_id(logical_link);
        /* TODO: create API function to get state */
        if (pIFObjectPtr[uPhysicalEthernetId]->DHCPContextState.tDHCPCurrentState < BOUND){
          if (uDHCPBoundCount[uPhysicalEthernetId] < uDHCPTimeoutMax){ /*  prevent overflows */
            uDHCPBoundCount[uPhysicalEthernetId]++;   /* keep track of how long we've been unbound */
          }
        } else {
          uDHCPBoundCount[uPhysicalEthernetId] = 0; /* reset the counter if we have progressed passed the BOUND state at any point */
        }
        if (uDHCPBoundCount[uPhysicalEthernetId] >=  uDHCPTimeoutMax){
          uDHCPBoundTimeout++;
        }
      }
      /*
       * if we timeout on all the interfaces, line up a reset
       */
      //if(uDHCPBoundTimeout >= NUM_ETHERNET_INTERFACES){
      if(uDHCPBoundTimeout >= num_links){
        log_printf(LOG_SELECT_DHCP, LOG_LEVEL_ERROR, "DHCP [..] Timed out on all interfaces after %dms!\r\n", uDHCPTimeoutMax * 100);

        /* Keep track of the number of DHCP caused reboots */
        if (PMemState != PMEM_RETURN_ERROR){
          ret = PersistentMemory_ReadByte(DHCP_RECONFIG_COUNT_INDEX, &uDHCPReconfigCount);
          if (PMEM_RETURN_OK == ret){
            uDHCPReconfigCount = uDHCPReconfigCount + 1;
            ret = PersistentMemory_WriteByte(DHCP_RECONFIG_COUNT_INDEX, uDHCPReconfigCount);
            if (PMEM_RETURN_OK == ret){
              log_printf(LOG_SELECT_DHCP, LOG_LEVEL_INFO, "DHCP [..] increment reset count to %d\r\n", uDHCPReconfigCount);
            }
          }
          /* print error message if pmem read or write fails */
          if (ret != PMEM_RETURN_OK){
            log_printf(LOG_SELECT_DHCP, LOG_LEVEL_ERROR, "DHCP [..] failed to store reset count in pmem\r\n");
          }
        }

        /*********************REBOOT THE FPGA****************************/
        //sudo_reboot_now();
        /****************************************************************/

        log_printf(LOG_SELECT_DHCP, LOG_LEVEL_WARN, "DHCP [..] resetting firmware\r\n");

        /* just wait a little while to enable serial port to finish writing out */
        Delay(100000); /* 100ms */

        WriteBoardRegister(C_WR_BRD_CTL_STAT_0_ADDR, 1 << 30);    /* set bit 30 of board ctl reg 0 to reset the fw */

#if 0
        if (((ReadBoardRegister(C_RD_VERSION_ADDR) & 0xff000000) >> 24) == 0){
          /* if it's a toolflow image */
          SetOutputMode(SDRAM_READ_MODE, 0x0); /* Release flash bus when about to reboot */
          ResetSdramReadAddress();
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_WARN, "REBOOT - toolflow image: reconfigure from SDRAM\r\n");
          AboutToBootFromSdram();
        }
        uDoReboot = REBOOT_REQUESTED;
#endif

      }
    }
#endif




    /* link monitor and recovery task */
    if (uFlagRunTask_LinkRecovery){
      uFlagRunTask_LinkRecovery = 0;

      for (logical_link = 0; logical_link < num_links; logical_link++){
        uPhysicalEthernetId = get_physical_interface_id(logical_link);

        if_link_recovery_task(uPhysicalEthernetId, uDHCPTimeoutMax);
      }

    }



#endif

    //----------------------------------------------------------------------------//
    //  DUMP INTERFACE COUNTERS AND OTHER USEFUL DEBUG INFO TO TERMINAL           //
    //  Triggered on ICMP ping and timer                                          //
    //----------------------------------------------------------------------------//
    if (uFlagRunTask_Diagnostics){
      uFlagRunTask_Diagnostics = 0;

      /* also print some other useful info for debugging */
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "Register C_RD_ETH_IF_LINK_UP_ADDR: 0x%08x\r\n", ReadBoardRegister(C_RD_ETH_IF_LINK_UP_ADDR));

      //for(uPhysicalEthernetId = 0; uPhysicalEthernetId < NUM_ETHERNET_INTERFACES; uPhysicalEthernetId++){
      for (logical_link = 0; logical_link < num_links; logical_link++){
        uPhysicalEthernetId = get_physical_interface_id(logical_link);
        PrintInterfaceCounters(pIFObjectPtr[uPhysicalEthernetId]);
      }
    }


    /* set the 40gbe interface link and dhcp status in firmware LED-manager register */
    u8 interface_id;

    if(uFlagRunTask_WriteLEDStatus){
      uFlagRunTask_WriteLEDStatus = 0;
      uFrontPanelLedsValue = 0;

      for (interface_id = 1; interface_id <= 4;  interface_id++){     /* loop through physical interface id's */
        /* check if the 40gbe core [1 to 4] is compiled into the firmware */
        if (IF_ID_PRESENT == check_interface_valid_quietly(interface_id)){
          /* check the link status */
          if (pIFObjectPtr[interface_id]->uIFLinkStatus == LINK_UP){
            uFrontPanelLedsValue = uFrontPanelLedsValue | 1 << ((interface_id * 2) - 1);
            /* check the dhcp status */
            if (DHCP_RETURN_TRUE == uDHCPGetBoundStatus(pIFObjectPtr[interface_id])){
              //uFrontPanelLedsValue = uFrontPanelLedsValue | 1 << ((interface_id * 2) - 2);
              uFrontPanelLedsValue = uFrontPanelLedsValue | 1;    /* C_WR_FRONT_PANEL_STAT_LED_ADDR(0) now monitors dhcp
                                                                     on all interfaces */
            }
          }
        }
      }

      WriteBoardRegister(C_WR_FRONT_PANEL_STAT_LED_ADDR, uFrontPanelLedsValue);
    }

    /* General 100ms task */
    if (uTick_100ms){
      uTick_100ms = 0;

      /* tick uptime every second */
      if (post_scale < 9){
        post_scale++;
      } else {
        post_scale = 0;
        incr_microblaze_uptime_seconds();
      }
    }


    if(uFlagRunTask_Aux){
      uFlagRunTask_Aux = 0;
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, ">>>>>>>>>>>aux timer<<<<<<<<<<<<<<<\r\n");
    }


    if (uDoReboot == REBOOT_REQUESTED)
    {
      // Only do a reboot if all the Ethernet interfaces are no longer part of groups
      uOKToReboot = 1;

#if 0
      for (uPhysicalEthernetId = 0; uPhysicalEthernetId < NUM_ETHERNET_INTERFACES; uPhysicalEthernetId++)
      {
        if (uIGMPState[uPhysicalEthernetId] != IGMP_STATE_NOT_JOINED){
          uOKToReboot = 0;
        }
      }
#endif
      if (uOKToReboot == 1){
        /* just wait a little while to enable serial port to finish writing out */
        Delay(100000); /* 100ms */
        IcapeControllerInSystemReconfiguration();
      }

    }

    //----------------------------------------------------------------------------//
    // very (very,very) simple serial character parser for configuring log-level  //
    //----------------------------------------------------------------------------//
    ReceivedCount = XUartLite_Recv(&UartLite, &RecvBuffer, 1);
		if (ReceivedCount){
      cli_sm(RecvBuffer);
		}

    /* to test the watchdog timer - uncomment the following */
    /***************/
    //while(1);
    /***************/

    // Pat the watchdogs

    /* Firmware watchdog */
    /* Read register */
    uKeepAliveReg = ReadBoardRegister(C_RD_UBLAZE_ALIVE_ADDR);

    /* Set the bit in register */
    uKeepAliveReg = uKeepAliveReg | 0x1;

    /* Write back to the register */
    WriteBoardRegister(C_WR_UBLAZE_ALIVE_ADDR, uKeepAliveReg);


    /* Microblaze watchdog */
    XWdtTb_RestartWdt(& WatchdogTimer);
  }

  /* should never reach here */
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "---Exiting main---\r\n");
  //Xil_DCacheDisable();
  //Xil_ICacheDisable();
  return 0;
}


static int vSendDHCPMsg(struct sIFObject *pIFObjectPtr, void *pUserData){
  u8 uLocalEthernetId;
  u8 uReturnValue;
  u32 uLocalSize;
  u16 *pLocalBuffer = NULL;
  u32 uLocalIndex;
  struct sDHCPObject *pDHCPObjectPtr;

  if (pIFObjectPtr == NULL){
    return -1;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    return -1;
  }

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (pDHCPObjectPtr == NULL){
    return -1;
  }

  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return -1;
  }

  uLocalSize = (u32) pIFObjectPtr->uMsgSize;    /* bytes */

  pLocalBuffer = (u16 *) pIFObjectPtr->pUserTxBufferPtr;
  if(pLocalBuffer == NULL){
    return XST_FAILURE;
  }

  uLocalSize = uLocalSize >> 1;     /* 16bit words */

  for (uLocalIndex = 0; uLocalIndex < uLocalSize; uLocalIndex++){
    pLocalBuffer[uLocalIndex] = Xil_EndianSwap16(pLocalBuffer[uLocalIndex]);
  }

  uLocalEthernetId = pIFObjectPtr->uIFEthernetId;

  uLocalSize = uLocalSize >> 1;   /*  32-bit words */
  uReturnValue = TransmitHostPacket(uLocalEthernetId, (u32 *) pLocalBuffer, uLocalSize);
  if (uReturnValue == XST_SUCCESS){
    /* log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "DHCP [%02x] sent DHCP packet with xid 0x%x\r\n", uLocalEthernetId, pDHCPObjectPtr->uDHCPXidCached); */
    IFCounterIncr(pIFObjectPtr, TX_UDP_DHCP_OK);
  } else {
    log_printf(LOG_SELECT_DHCP, LOG_LEVEL_ERROR, "DHCP [%02x] FAILED to send DHCP packet with xid 0x%x\r\n", uLocalEthernetId, pDHCPObjectPtr->uDHCPXidCached);
    IFCounterIncr(pIFObjectPtr, TX_UDP_DHCP_ERR);
  }

  return uReturnValue;
}


static int vSetInterfaceConfig(struct sIFObject *pIFObjectPtr, void *pUserData){
  u32 ip = 0;
  u32 netmask = 0;
  u8 id;
  struct sDHCPObject *pDHCPObjectPtr;

  if (pIFObjectPtr == NULL){
    return -1;
  }

  if (pIFObjectPtr->uIFMagic != IF_MAGIC){
    return -1;
  }

  pDHCPObjectPtr = &(pIFObjectPtr->DHCPContextState);

  if (pDHCPObjectPtr == NULL){
    return -1;
  }

  if (pDHCPObjectPtr->uDHCPMagic != DHCP_MAGIC){
    return -1;
  }

  id = pIFObjectPtr->uIFEthernetId;
  ip = (pDHCPObjectPtr->arrDHCPAddrYIPCached[0] << 24) |
    (pDHCPObjectPtr->arrDHCPAddrYIPCached[1] << 16) |
    (pDHCPObjectPtr->arrDHCPAddrYIPCached[2] << 8 ) |
    pDHCPObjectPtr->arrDHCPAddrYIPCached[3];

  netmask = (pDHCPObjectPtr->arrDHCPAddrSubnetMask[0] << 24) |
    (pDHCPObjectPtr->arrDHCPAddrSubnetMask[1] << 16) |
    (pDHCPObjectPtr->arrDHCPAddrSubnetMask[2] << 8 ) |
    pDHCPObjectPtr->arrDHCPAddrSubnetMask[3];

#if 0
  /* convert ip to string and cache for later use / printing */
  if (uIPV4_ntoa((char *) (pIFObjectPtr->stringIFAddrIP), ip) != 0){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "DHCP [%02x] Unable to convert IP %x to string.\r\n", id, ip);
  } else {
    pIFObjectPtr->stringIFAddrIP[15] = '\0';
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "DHCP [%02x] Setting IP address to: %s\r\n", id, pIFObjectPtr->stringIFAddrIP);
  }

  /* convert netmask to string and cache for later use / printing */
  if (uIPV4_ntoa((char *) (pIFObjectPtr->stringIFAddrNetmask), netmask) != 0){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "DHCP [%02x] Unable to convert Netmask %x to string.\r\n", id, netmask);
  } else {
    pIFObjectPtr->stringIFAddrNetmask[15] = '\0';
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "DHCP [%02x] Setting Netmask address to: %s\r\n", id, pIFObjectPtr->stringIFAddrNetmask);
  }
#endif

  //uEthernetFabricIPAddress[id] = ip;

  /* Cache the ip and netmask in the IF Object layer */
#if 0
  memcpy(pIFObjectPtr->arrIFAddrIP, pDHCPObjectPtr->arrDHCPAddrYIPCached, 4);
  memcpy(pIFObjectPtr->arrIFAddrNetmask, pDHCPObjectPtr->arrDHCPAddrSubnetMask, 4);
#endif

  IFConfig(pIFObjectPtr, ip, netmask);

  /* uEthernetSubnet[id] = (ip & 0xFFFFFF00); */
  /* uEthernetSubnet[id] = (ip & netmask); */
  pIFObjectPtr->uIFEthernetSubnet = (ip & netmask);

  uEthernetGatewayIPAddress[id] = (pDHCPObjectPtr->arrDHCPAddrRoute[0] << 24) |
    (pDHCPObjectPtr->arrDHCPAddrRoute[1] << 16) |
    (pDHCPObjectPtr->arrDHCPAddrRoute[2] << 8 ) |
    pDHCPObjectPtr->arrDHCPAddrRoute[3];

  ProgramARPCacheEntry(id, (ip & 0xFF), uEthernetFabricMacHigh[id], ((uEthernetFabricMacMid[id] << 16) | uEthernetFabricMacLow[id]));

  SetFabricSourceIPAddress(id, ip);

  SetFabricNetmask(id, netmask);

  SetFabricGatewayARPCacheAddress(id, pDHCPObjectPtr->arrDHCPAddrRoute[3]);

  pIFObjectPtr->uIFEnableArpRequests = ARP_REQUESTS_ENABLE;

  /* legacy dhcp states */
  uDHCPState[id] = DHCP_STATE_COMPLETE;

  EnableFabricInterface(id, 1);
  uFlagRunTask_LLDP[id] = 1;

  /* cache the ip in persistent memory for DHCP request on next reconfigure cycle */
  /* TODO: should we check the state of persistent memory again (?) - perhaps a
   * partial check of the following byte indices only...
   */
  if (id == 1){
    PersistentMemory_WriteByte(DHCP_CACHED_IP_OCT0_INDEX, pDHCPObjectPtr->arrDHCPAddrYIPCached[0]);
    PersistentMemory_WriteByte(DHCP_CACHED_IP_OCT1_INDEX, pDHCPObjectPtr->arrDHCPAddrYIPCached[1]);
    PersistentMemory_WriteByte(DHCP_CACHED_IP_OCT2_INDEX, pDHCPObjectPtr->arrDHCPAddrYIPCached[2]);
    PersistentMemory_WriteByte(DHCP_CACHED_IP_OCT3_INDEX, pDHCPObjectPtr->arrDHCPAddrYIPCached[3]);

    /* also cache the netmask */
    PersistentMemory_WriteByte(DHCP_CACHED_MASK_OCT0_INDEX, pDHCPObjectPtr->arrDHCPAddrSubnetMask[0]);
    PersistentMemory_WriteByte(DHCP_CACHED_MASK_OCT1_INDEX, pDHCPObjectPtr->arrDHCPAddrSubnetMask[1]);
    PersistentMemory_WriteByte(DHCP_CACHED_MASK_OCT2_INDEX, pDHCPObjectPtr->arrDHCPAddrSubnetMask[2]);
    PersistentMemory_WriteByte(DHCP_CACHED_MASK_OCT3_INDEX, pDHCPObjectPtr->arrDHCPAddrSubnetMask[3]);

    /* and cache the gateway */
    PersistentMemory_WriteByte(DHCP_CACHED_GW_OCT0_INDEX, pDHCPObjectPtr->arrDHCPAddrRoute[0]);
    PersistentMemory_WriteByte(DHCP_CACHED_GW_OCT1_INDEX, pDHCPObjectPtr->arrDHCPAddrRoute[1]);
    PersistentMemory_WriteByte(DHCP_CACHED_GW_OCT2_INDEX, pDHCPObjectPtr->arrDHCPAddrRoute[2]);
    PersistentMemory_WriteByte(DHCP_CACHED_GW_OCT3_INDEX, pDHCPObjectPtr->arrDHCPAddrRoute[3]);

    PersistentMemory_WriteByte(DHCP_CACHED_IP_STATE_INDEX, 1);

#if 0
    if (uIGMPState[id] == IGMP_STATE_JOINED_GROUP){
      log_printf(LOG_SELECT_IFACE, LOG_LEVEL_INFO, "I/F  [%02x] resubscribing to previous multicast groups!\r\n", id);
      uIGMPSendMessage[id] = IGMP_SEND_MESSAGE;
      uCurrentIGMPMessage[id] = 0x0;
    }
#endif
  }

  /*
   *  If we were previously part of a multicast group, try to resend igmp
   *  subscriptions as soon as we obtain a lease. This will allow us to rejoin
   *  the multicast group if the link "flaps" or the switch is rebooted.
   */
  if (XST_FAILURE == uIGMPRejoinPrevGroup(id)){
    log_printf(LOG_SELECT_IGMP, LOG_LEVEL_ERROR, "IGMP [%02x] FAILED to rejoin multicast group\r\n", id);
  }

  return 0;
}

/* TODO: act upon these exceptions */
void DivByZeroException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Divide by zero exception\r\n");
}

void IBusException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Instruction AXI bus exception\r\n");
}

void DBusException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Data AXI bus exception\r\n");

  /* this exception was most likely caused by an illegal addressing of the wishbone bus which resulted in a bus timeout */
  set_error_flag(ERROR_AXI_DATA_BUS);
}

void StackViolationException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Stack violation exception\r\n");
}

void IllegalOpcodeException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Illegal opcode exception\r\n");
}

#if 0
void UnalignedAccessException(void *Data){
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Unaligned data access exception\r\n");
}
#endif
