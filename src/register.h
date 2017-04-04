
#ifndef REGISTER_H_
#define REGISTER_H_

/**------------------------------------------------------------------------------
*  FILE NAME            : register.h
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
*  This file contains the definitions and constants for accessing the registers
*  over the Wishbone bus.
* ------------------------------------------------------------------------------*/

#include <stdio.h>
#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"

void WriteBoardRegister(u32 uWriteAddress, u32 uWriteData);
u32 ReadBoardRegister(u32 uReadAddress);
void WriteDSPRegister(u32 uWriteAddress, u32 uWriteData);
u32 ReadDSPRegister(u32 uReadAddress);

#endif
