/**----------------------------------------------------------------------------
*   FILE:       diagnosics.h
*   BRIEF:      API of various diagnostic functions.
*
*   DATE:       MARCH 2018
*
*   COMPANY:    SKA SA
*   AUTHOR:     R van Wyk
*
*------------------------------------------------------------------------------*/

/* vim settings: "set sw=2 ts=2 expandtab autoindent" */

#ifndef _DIAG_H_
#define _DIAG_H_

#include "if.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Network */
void PrintInterfaceCounters(struct sIFObject *pIFObj);

#ifndef PRUNE_CODEBASE_DIAGNOSTICS
void print_interface_internals(u8 physical_interface_id);
void print_dhcp_internals(u8 physical_interface_id);
#endif

/* General */
void PrintVersionInfo();
const char *GetVersionInfo();
void ReadAndPrintPeralexSerial();
void ReadAndPrintFPGADNA();

#ifdef __cplusplus
}
#endif
#endif
