/**----------------------------------------------------------------------------
 *   FILE:       wdt.c
 *   BRIEF:      Functionality to initialize the watchdog timer
 *
 *   DATE:       2018
 *
 *   COMPANY:    SKA SA
 *
 *------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

#include <xwdttb.h>
#include <xstatus.h>

#include "wdt.h"
#include "logging.h"

void init_wdt(XWdtTb *WatchdogTimer){
  int ret;
  XWdtTb_Config *WatchdogTimerConfig;

  if (NULL == WatchdogTimer){
    return;
  }

  /* Lookup the WDT configuration */
  WatchdogTimerConfig = XWdtTb_LookupConfig(XPAR_WDTTB_0_DEVICE_ID);
  if (WatchdogTimerConfig == NULL){
    log_printf(LOG_LEVEL_ERROR, "WDT  [..] failed to find wdt config\r\n");
    return;
  }

  log_printf(LOG_LEVEL_DEBUG, "WDT  [..] read 1...reg TWCSR0 is 0x%08x\r\n", XWdtTb_ReadReg(WatchdogTimerConfig->BaseAddr, XWT_TWCSR0_OFFSET));
  log_printf(LOG_LEVEL_DEBUG, "WDT  [..] read 2...reg TWCSR0 is 0x%08x\r\n", XWdtTb_ReadReg(WatchdogTimerConfig->BaseAddr, XWT_TWCSR0_OFFSET));
  log_printf(LOG_LEVEL_DEBUG, "WDT  [..] reg TBR is 0x%08x\r\n", XWdtTb_ReadReg(WatchdogTimerConfig->BaseAddr, XWT_TBR_OFFSET));

  /* Initialize the AXI Timebase Watchdog Timer core */
  ret = XWdtTb_CfgInitialize(WatchdogTimer, WatchdogTimerConfig, WatchdogTimerConfig->BaseAddr);
  if (ret == XST_DEVICE_IS_STARTED){
    log_printf(LOG_LEVEL_ERROR, "WDT  [..] failed to initialize the wdt since it is already running\r\n");

    /* check the WRS & WDS bits then restart */
    /* must check this before next stop/re-configure cycle */
    if (XWdtTb_IsWdtExpired(WatchdogTimer) == TRUE){
      log_printf(LOG_LEVEL_INFO, "WDT  [..] previous reset was a result of a watchdog timeout!\r\n");
    }

    ret = XWdtTb_Stop(WatchdogTimer);
    if (ret == XST_NO_FEATURE){
      log_printf(LOG_LEVEL_WARN, "WDT  [..] the watchdog timer cannot be stopped, possibly due to 'enable-once' setting\r\n");
    } else if (ret == XST_SUCCESS){
      log_printf(LOG_LEVEL_DEBUG, "WDT  [..] watchdog stopped\r\n");
      XWdtTb_CfgInitialize(WatchdogTimer, WatchdogTimerConfig, WatchdogTimerConfig->BaseAddr);
    }

    //XWdtTb_RestartWdt(WatchdogTimer);
    XWdtTb_Start(WatchdogTimer);
    log_printf(LOG_LEVEL_DEBUG, "WDT  [..] restart watchdog timer!\r\n");
  } else {
    if (XWdtTb_IsWdtExpired(WatchdogTimer) == TRUE){
      log_printf(LOG_LEVEL_INFO, "WDT  [..] previous reset was a result of a watchdog timeout!\r\n");
    }
    /* perform a wdt self-test to ensure the timebase is incrementing */
    ret = XWdtTb_SelfTest(WatchdogTimer);
    if (ret != XST_SUCCESS){
      log_printf(LOG_LEVEL_ERROR, "WDT  [..] self-test failed\r\n");
    } else {
      /* finally, start the watchdog. The timebase automatically reset*/
      XWdtTb_Start(WatchdogTimer);
      log_printf(LOG_LEVEL_INFO, "WDT  [..] starting the watchdog timer...\r\n");
    }
  }

  /* read wdt registers after setup */
  log_printf(LOG_LEVEL_DEBUG, "WDT  [..] reg TWCSR0 is 0x%08x\r\n", XWdtTb_ReadReg(WatchdogTimer->Config.BaseAddr, XWT_TWCSR0_OFFSET));
  log_printf(LOG_LEVEL_DEBUG, "WDT  [..] reg TBR is 0x%08x\r\n", XWdtTb_ReadReg(WatchdogTimer->Config.BaseAddr, XWT_TBR_OFFSET));
}
