/**------------------------------------------------------------------------------
 *  FILE NAME            : icape_controller.c
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
 *  This file contains the implementation of functions to access the ICAPE2
 *  controller over the Wishbone bus.
 * ------------------------------------------------------------------------------*/

#include <xil_io.h>
#include <xparameters.h>

#include "icape_controller.h"
#include "constant_defs.h"

//=================================================================================
//  IcapeControllerWriteWord
//--------------------------------------------------------------------------------
//  This method writes a word to the ICAPE controller over the Wishbone bus.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uWriteData    IN  Data to write
//
//  Return
//  ------
//  None
//=================================================================================
void IcapeControllerWriteWord(u32 uWriteData)
{
  u32 uReg;

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_DATA_REG_ADDRESS, uWriteData);

  uReg = ICAPE_CONTROLLER_WRITE;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS, uReg);

  // Toggle start bit
  uReg = uReg | ICAPE_CONTROLLER_START_TRANS;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS, uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS);
  }while ((uReg & ICAPE_CONTROLLER_TRANS_COMPLETE) == 0x0);

  // Operation cannot be stalled so no need for a timeout here

}

//=================================================================================
//  IcapeControllerReadWord
//--------------------------------------------------------------------------------
//  This method reads a word from the ICAPE controller over the Wishbone bus.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  Data read
//=================================================================================
u32 IcapeControllerReadWord()
{
  u32 uReg;

  uReg = ICAPE_CONTROLLER_READ;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS, uReg);

  // Toggle start bit
  uReg = uReg | ICAPE_CONTROLLER_START_TRANS;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS, uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_CTL_REG_ADDRESS);
  }while ((uReg & ICAPE_CONTROLLER_TRANS_COMPLETE) == 0x0);

  // Operation cannot be stalled so no need for a timeout here

  // Do read of data and return
  return (Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + ICAPE_DATA_REG_ADDRESS));

}

//=================================================================================
//  IcapeControllerInSystemReconfiguration
//--------------------------------------------------------------------------------
//  This method triggers an FPGA reconfiguration using the ICAPE.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void IcapeControllerInSystemReconfiguration()
{
  IcapeControllerWriteWord(ICAPE_DUMMY_WORD);
  IcapeControllerWriteWord(ICAPE_SYNC_WORD);
  IcapeControllerWriteWord(ICAPE_TYPE_1_NO_OP);
  IcapeControllerWriteWord(ICAPE_TYPE_1_WRITE_1_TO_WBSTAR);
  IcapeControllerWriteWord(ICAPE_WBSTAR);
  IcapeControllerWriteWord(ICAPE_TYPE_1_WRITE_1_TO_CMD);
  IcapeControllerWriteWord(ICAPE_IPROG);
  IcapeControllerWriteWord(ICAPE_TYPE_1_NO_OP);
}
