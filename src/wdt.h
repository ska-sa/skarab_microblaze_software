/**----------------------------------------------------------------------------
*   FILE:       wdt.h
*   BRIEF:      API function to initialize the watchdog timer.
*
*   DATE:       2018
*
*   COMPANY:    SKA SA
*
*------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

#ifndef _WDT_H_
#define _WDT_H_

#include <xwdttb.h>

void init_wdt(XWdtTb *WatchdogTimer);

#endif
