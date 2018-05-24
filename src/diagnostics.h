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

void PrintInterfaceCounters(struct sIFObject *pIFObj);

#ifdef __cplusplus
}
#endif
#endif
