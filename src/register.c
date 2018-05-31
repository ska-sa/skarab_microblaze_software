/**------------------------------------------------------------------------------
 *  FILE NAME            : register.c
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
 *  This file contains the implementation of functions to registers over the
 *  Wishbone bus.
 * ------------------------------------------------------------------------------*/

#include <xil_io.h>
#include <xparameters.h>

#include "register.h"
#include "constant_defs.h"

//=================================================================================
//  WriteBoardRegister
//--------------------------------------------------------------------------------
//  This method writes a board register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uWriteAddress IN  Register to write to
//  uWriteData    IN  Data to write
//
//  Return
//  ------
//  None
//=================================================================================
void WriteBoardRegister(u32 uWriteAddress, u32 uWriteData)
{

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + BOARD_REGISTER_ADDR + uWriteAddress, uWriteData);
  uWriteBoardShadowRegs[uWriteAddress >> 2] = uWriteData;

}

//=================================================================================
//  ReadBoardRegister
//--------------------------------------------------------------------------------
//  This method reads a board register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uReadAddress  IN  Register to read from
//
//  Return
//  ------
//  Data read
//=================================================================================
u32 ReadBoardRegister(u32 uReadAddress)
{

  return (Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + BOARD_REGISTER_ADDR + uReadAddress));

}

//=================================================================================
//  WriteDSPRegister
//--------------------------------------------------------------------------------
//  This method writes a DSP register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uWriteAddress IN  Register to write to
//  uWriteData    IN  Data to write
//
//  Return
//  ------
//  None
//=================================================================================
void WriteDSPRegister(u32 uWriteAddress, u32 uWriteData)
{

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + DSP_REGISTER_ADDR + uWriteAddress, uWriteData);

}

//=================================================================================
//  ReadDSPRegister
//--------------------------------------------------------------------------------
//  This method reads a DSP register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uReadAddress  IN  Register to read from
//
//  Return
//  ------
//  Data read
//=================================================================================
u32 ReadDSPRegister(u32 uReadAddress)
{

  return (Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + DSP_REGISTER_ADDR + uReadAddress));

}
