
#ifndef ICAPE_CONTROLLER_H_
#define ICAPE_CONTROLLER_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : icape_controller.h
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
*  This file contains the definitions and constants for accessing the ICAPE2
*  controller over the Wishbone bus.
* ------------------------------------------------------------------------------*/

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ICAPE_DATA_REG_ADDRESS  0x4010
#define ICAPE_CTL_REG_ADDRESS 0x4014

#define ICAPE_CONTROLLER_READ       0x1
#define ICAPE_CONTROLLER_WRITE      0x0
#define ICAPE_CONTROLLER_START_TRANS  0x2
#define ICAPE_CONTROLLER_TRANS_COMPLETE 0x1

// Bit swapping done by the firmware so this is correct
#define ICAPE_DUMMY_WORD        0xFFFFFFFF
#define ICAPE_SYNC_WORD         0xAA995566
#define ICAPE_TYPE_1_NO_OP        0x20000000
#define ICAPE_TYPE_1_WRITE_1_TO_WBSTAR  0x30020001
#define ICAPE_WBSTAR          0x00000000
#define ICAPE_TYPE_1_WRITE_1_TO_CMD   0x30008001
#define ICAPE_IPROG           0x0000000F

void IcapeControllerWriteWord(u32 uWriteData);
u32 IcapeControllerReadWord();

void IcapeControllerInSystemReconfiguration();

#ifdef __cplusplus
}
#endif
#endif
