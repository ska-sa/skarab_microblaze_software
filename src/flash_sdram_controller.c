/**------------------------------------------------------------------------------
 *  FILE NAME            : flash_sdram_controller.c
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
 *  This file contains the implementation of functions to access the flash and
 *  SDRAM controller over the Wishbone bus.
 * ------------------------------------------------------------------------------*/

#include <xil_io.h>
#include <xparameters.h>
#include <xstatus.h>

#include "flash_sdram_controller.h"
#include "constant_defs.h"
#include "delay.h"
#include "logging.h"

//=================================================================================
//  SetOutputMode
//--------------------------------------------------------------------------------
//  This method sets the output mode for the FLASH SDRAM controller.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uOutputMode IN    Output mode
//
//  Return
//  ------
//  None
//=================================================================================
void SetOutputMode(u8 uOutputMode, u8 uFlashOutputEnable)
{
  unsigned uReg;

  if (uFlashOutputEnable == 1)
    uReg = uOutputMode | FLASH_OUTPUT_ENABLE;
  else
    uReg = uOutputMode;

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_MODE_REG_ADDRESS, uReg);
}

//=================================================================================
//  SetUpperAddressBits
//--------------------------------------------------------------------------------
//  This method extracts the upper address bits and writes them to the register
//  in the FLASH SDRAM controller. NOTE: Wishbone bus uses byte addresses so
//  lowest two address bits are dropped. Address bits 13 downto 2 index directly
//  and address bits 30 downto 14 are set via the upper address register. Flash
//  addresses must be multiplied by 4 before being used here.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Address (upper address bits are extracted and written)
//
//  Return
//  ------
//  None
//=================================================================================
void SetUpperAddressBits(u32 uAddress)
{
  u32 uReg;

  uReg = (uAddress >> 14) & 0x1FFFF;

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_UPPER_ADDRESS_REG_ADDRESS, uReg);
}

//=================================================================================
//  ClearSdram
//--------------------------------------------------------------------------------
//  This method instructs the SDRAM controller on the Spartan 3AN to flash all
//  local buffering (effectively emptying SDRAM).
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void ClearSdram()
{
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, CLEAR_SDRAM);
  Delay(10);
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
}

//=================================================================================
//  FinishedWritingToSdram
//--------------------------------------------------------------------------------
//  This method instructs the SDRAM controller on the Spartan 3AN that the complete
//  Virtex 7 FPGA image has now been programmed to the SDRAM.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void FinishedWritingToSdram()
{
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, FINISHED_WRITING_TO_SDRAM);
  Delay(10);
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
}

//=================================================================================
//  AboutToBootFromSdram
//--------------------------------------------------------------------------------
//  This method instructs the SDRAM controller on the Spartan 3AN that the Virtex 7
//  is going to be reconfigured soon and that it is to boot from SDRAM instead of
//  the NOR flash.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void AboutToBootFromSdram()
{
  u32 uReg;

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, ABOUT_TO_BOOT_FROM_SDRAM);

  // Check that Spartan 3AN is ready
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS);
  }while((uReg & BOOTING_FROM_SDRAM) == 0x0);

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
}

//=================================================================================
//  FinishedBootingFromSdram
//--------------------------------------------------------------------------------
//  This method instructs the SDRAM controller on the Spartan 3AN that the Virtex 7
//  is finished configuring. This will always be called as part of the start up code
//  of the Microblaze.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void FinishedBootingFromSdram()
{
  u32 uReg;

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, FINISHED_BOOTING_FROM_SDRAM);

  // Check that Spartan 3AN is ready
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS);
  }while((uReg & BOOTING_FROM_SDRAM) != 0x0);

  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
}

//=================================================================================
//  ResetSdramReadAddress
//--------------------------------------------------------------------------------
//  This method instructs the SDRAM controller on the Spartan 3AN to reset the SDRAM
//  read address to 0. This allows booting again from the image programmed in SDRAM
//  without having to program the image again.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void ResetSdramReadAddress()
{
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, RESET_SDRAM_READ_ADDRESS);
  Delay(10);
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
}

//=================================================================================
//  ClearEthernetStatics
//--------------------------------------------------------------------------------
//  The 1GBE fabric interface is used to funnel high speed SDRAM programming data
//  through the FLASH SDRAM controller to the SDRAM. Basic statistics are kept on
//  the packets received and this method clears those statistics.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void ClearEthernetStatics()
{
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_GBE_STATS_REG_ADDRESS, 0x1);
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_GBE_STATS_REG_ADDRESS, 0x0);
}

//=================================================================================
//  GetNumEthernetFrames
//--------------------------------------------------------------------------------
//  The 1GBE fabric interface is used to funnel high speed SDRAM programming data
//  through the FLASH SDRAM controller to the SDRAM. Basic statistics are kept on
//  the packets received and this method returns the number of Ethernet frames received.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  Total number of Ethernet frames received.
//=================================================================================
u16 GetNumEthernetFrames()
{
  u32 uReg;
  uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_GBE_STATS_REG_ADDRESS);

  return (uReg & 0xFFFF);
}

//=================================================================================
//  GetNumEthernetBadFrames
//--------------------------------------------------------------------------------
//  The 1GBE fabric interface is used to funnel high speed SDRAM programming data
//  through the FLASH SDRAM controller to the SDRAM. Basic statistics are kept on
//  the packets received and this method returns the number of bad Ethernet frames
//  received.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  Total number of bad Ethernet frames received.
//=================================================================================
u8 GetNumEthernetBadFrames()
{
  u32 uReg;
  uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_GBE_STATS_REG_ADDRESS);

  return ((uReg >> 16) & 0xFF);
}

//=================================================================================
//  GetNumEthernetOverloadFrames
//--------------------------------------------------------------------------------
//  The 1GBE fabric interface is used to funnel high speed SDRAM programming data
//  through the FLASH SDRAM controller to the SDRAM. Basic statistics are kept on
//  the packets received and this method returns the number of overload Ethernet frames
//  received.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  Total number of overload Ethernet frames received.
//=================================================================================
u8 GetNumEthernetOverloadFrames()
{
  u32 uReg;
  uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_GBE_STATS_REG_ADDRESS);

  return ((uReg >> 24) & 0xFF);
}

//=================================================================================
//  WriteFlashWord
//--------------------------------------------------------------------------------
//  This method does a low level asynchronous write on the flash interface.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Address to write to
//  uData   IN    Data to write
//
//  Return
//  ------
//  None
//=================================================================================
void WriteFlashWord(u32 uAddress, u16 uData)
{
  u32 uAddressLower = (uAddress << 2) & 0x3FFC;

  // Start by setting the upper address bits
  SetUpperAddressBits(uAddress << 2);
  Xil_Out16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + uAddressLower, uData);
}

//=================================================================================
//  ReadFlashWord
//--------------------------------------------------------------------------------
//  This method does a low level asynchronous read on the flash interface.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Address to write to
//
//  Return
//  ------
//  Data read from flash interface
//=================================================================================
u16 ReadFlashWord(u32 uAddress)
{
  u32 uAddressLower = (uAddress << 2) & 0x3FFC;

  // Start by setting the upper address bits
  SetUpperAddressBits(uAddress << 2);
  return (Xil_In16(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + uAddressLower));
}

//=================================================================================
//  ReadStatusRegisterCmd
//--------------------------------------------------------------------------------
//  This method reads the FLASH status register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uPartitionAddress IN    Device partition address
//
//  Return
//  ------
//  Status register value
//=================================================================================
u16 ReadStatusRegisterCmd(u32 uPartitionAddress)
{
  WriteFlashWord(uPartitionAddress, FLASH_READ_STATUS_REGISTER);
  return (ReadFlashWord(uPartitionAddress));
}

//=================================================================================
//  ClearStatusRegisterCmd
//--------------------------------------------------------------------------------
//  This method clears the FLASH status register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Device address
//
//  Return
//  ------
//  None
//=================================================================================
void ClearStatusRegisterCmd(u32 uAddress)
{
  // Different to previous flash family
  WriteFlashWord(uAddress, FLASH_CLEAR_STATUS_REGISTER);
}

//=================================================================================
//  ProgramReadConfigurationRegisterCmd
//--------------------------------------------------------------------------------
//  This method programs the FLASH Read Configuration register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uRCRValue IN    Read configuration register setting
//
//  Return
//  ------
//  None
//=================================================================================
void ProgramReadConfigurationRegisterCmd(u32 uRCRValue)
{
  log_printf(LOG_LEVEL_INFO, "Setting ASYNC read mode for flash.\r\n");
  WriteFlashWord(uRCRValue, FLASH_SET_CONFIG_REGISTER);
  WriteFlashWord(uRCRValue, FLASH_SET_CONFIG_REGISTER_CONFIRM);
}

//=================================================================================
//  ReadReadConfigurationRegisterCmd
//--------------------------------------------------------------------------------
//  This method reads the current value of the FLASH Read Configuration register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uPartitionAddress IN    Device partition address
//
//  Return
//  ------
//  Current value of the Read configuration register
//=================================================================================
u16 ReadReadConfigurationRegisterCmd(u32 uPartitionAddress)
{
  // Must check address for READ ID command
  WriteFlashWord(uPartitionAddress, FLASH_READ_DEVICE_IDENTIFIER);
  return (ReadFlashWord(uPartitionAddress + 0x5));
}

//=================================================================================
//  ProgramExtendedConfigurationRegisterCmd
//--------------------------------------------------------------------------------
//  This method programs the FLASH Extended Configuration register.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uExtendedValue  IN  Read configuration extended register setting
//
//  Return
//  ------
//  None
//=================================================================================
void ProgramExtendedConfigurationRegisterCmd(u32 uExtendedValue)
{
  WriteFlashWord(uExtendedValue, FLASH_SET_EXTENDED_CONFIG_REGISTER);
  WriteFlashWord(uExtendedValue, FLASH_SET_EXTENDED_CONFIG_REGISTER_CONFIRM);
}

//=================================================================================
//  ReadArrayCmd
//--------------------------------------------------------------------------------
//  This method puts the FLASH in read array mode.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uPartitionAddress IN    Device partition address
//
//  Return
//  ------
//  None
//=================================================================================
void ReadArrayCmd(u32 uPartitionAddress)
{
  WriteFlashWord(uPartitionAddress, FLASH_READ_ARRAY);
}

//=================================================================================
//  WordProgramCmd
//--------------------------------------------------------------------------------
//  This method sends the FLASH single word program command.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Device address
//  uData   IN    Data to write
//
//  Return
//  ------
//  None
//=================================================================================
void WordProgramCmd(u32 uAddress, u16 uData)
{
  WriteFlashWord(uAddress, FLASH_WORD_PROGRAM);
  WriteFlashWord(uAddress, uData);
}

//=================================================================================
//  BlockEraseCmd
//--------------------------------------------------------------------------------
//  This method sends the block erase command to the FLASH.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uBlockAddress IN  Device block address
//
//  Return
//  ------
//  None
//=================================================================================
void BlockEraseCmd(u32 uBlockAddress)
{
  WriteFlashWord(uBlockAddress, FLASH_BLOCK_ERASE);
  WriteFlashWord(uBlockAddress, FLASH_BLOCK_ERASE_CONFIRM);
}

//=================================================================================
//  UnlockBlockCmd
//--------------------------------------------------------------------------------
//  This method unlocks a block in the FLASH so that it can be erased or programmed.
//  All blocks are locked after a power up or reset.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uBlockAddress IN  Device block address
//
//  Return
//  ------
//  None
//=================================================================================
void UnlockBlockCmd(u32 uBlockAddress)
{
  WriteFlashWord(uBlockAddress,FLASH_UNLOCK_BLOCK);
  WriteFlashWord(uBlockAddress,FLASH_UNLOCK_BLOCK_CONFIRM);
}

//=================================================================================
//  ReadWord
//--------------------------------------------------------------------------------
//  This method reads a single word from the FLASH.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Device address to read
//
//  Return
//  ------
//  Data read at specified location
//=================================================================================
u16 ReadWord(u32 uAddress)
{
  ReadArrayCmd(uAddress & FLASH_PARTITION_ADDRESS_MASK);
  return (ReadFlashWord(uAddress));
}

//=================================================================================
//  ProgramWord
//--------------------------------------------------------------------------------
//  This method programs a single word into the FLASH.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Device address to program
//  uData   IN    Data to program
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ProgramWord(u32 uAddress, u16 uData)
{
  u32 uTimeOut = 0x0;
  u32 uStatusReg = 0x0;

  WordProgramCmd(uAddress, uData);

  do
  {
    uStatusReg = ReadStatusRegisterCmd(uAddress & FLASH_PARTITION_ADDRESS_MASK);
    Delay(1);
    uTimeOut ++;
  }
  while(((uStatusReg & FLASH_STATUS_WRITE_STATUS) == 0x0) && (uTimeOut < FLASH_PROGRAM_TIMEOUT));

  if (uTimeOut == FLASH_PROGRAM_TIMEOUT)
    return XST_FAILURE;

  if ((uStatusReg & FLASH_STATUS_PROGRAM_STATUS) != 0x0)
    return XST_FAILURE;

  if ((uStatusReg & FLASH_STATUS_PROGRAM_STATUS_2) != 0x0)
    return XST_FAILURE;

  if ((uStatusReg & FLASH_STATUS_VPP_STATUS) != 0x0)
    return XST_FAILURE;

  if ((uStatusReg & FLASH_STATUS_BLOCK_LOCK_ERROR) != 0x0)
    return XST_FAILURE;

  ClearStatusRegisterCmd(uAddress & FLASH_PARTITION_ADDRESS_MASK);

  return XST_SUCCESS;
}

//=================================================================================
//  ProgramBuffer
//--------------------------------------------------------------------------------
//  This method performs a buffered program into the FLASH.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uAddress  IN    Device address to program
//  puDataArray IN    Data to program
//  uNumWords IN    Number of words to program
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ProgramBuffer(u32 uAddress, u16 * puDataArray, u16 uTotalNumWords, u16 uNumWords, u16 uStartProgram, u16 uFinishProgram)
{
  u32 uTimeOut = 0x0;
  u32 uStatusReg = 0x0;
  u16 uWord;

  if (uNumWords > 512)
    return XST_FAILURE;

  if (uStartProgram == 1)
  {
    //log_printf(LOG_LEVEL_INFO, "Flash first programming\r\n");
    WriteFlashWord(uAddress, FLASH_BUFFERED_PROGRAM);

    WriteFlashWord(uAddress, uTotalNumWords - 1);
  }

  for (uWord = 0; uWord < uNumWords; uWord++)
  {
    WriteFlashWord(uAddress + uWord, puDataArray[uWord]);
  }

  if (uFinishProgram == 1)
  {
    //log_printf(LOG_LEVEL_INFO, "Flash finish programming\r\n");

    WriteFlashWord(uAddress, FLASH_BUFFERED_PROGRAM_CONFIRM);

    do
    {
      uStatusReg = ReadStatusRegisterCmd(uAddress & FLASH_PARTITION_ADDRESS_MASK);
      Delay(10);
      uTimeOut ++;
    }
    while(((uStatusReg & FLASH_STATUS_WRITE_STATUS) == 0x0) && (uTimeOut < FLASH_PROGRAM_TIMEOUT));

    if (uTimeOut == FLASH_PROGRAM_TIMEOUT)
    {
      log_printf(LOG_LEVEL_INFO, "Flash program timeout\r\n");
      return XST_FAILURE;
    }

    if ((uStatusReg & FLASH_STATUS_PROGRAM_STATUS) != 0x0)
    {
      log_printf(LOG_LEVEL_INFO, "Flash program status error\r\n");
      return XST_FAILURE;
    }

    if ((uStatusReg & FLASH_STATUS_VPP_STATUS) != 0x0)
    {
      log_printf(LOG_LEVEL_INFO, "Flash program vpp error\r\n");
      return XST_FAILURE;
    }

    if ((uStatusReg & FLASH_STATUS_BLOCK_LOCK_ERROR) != 0x0)
    {
      log_printf(LOG_LEVEL_INFO, "Flash program block lock error\r\n");
      return XST_FAILURE;
    }

    ClearStatusRegisterCmd(uAddress & FLASH_PARTITION_ADDRESS_MASK);

  }

  return XST_SUCCESS;
}

//=================================================================================
//  EraseBlock
//--------------------------------------------------------------------------------
//  This method erases a block in the FLASH.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uBlockAddress IN  Device address to erase
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int EraseBlock(u32 uBlockAddress)
{
  u32 uTimeOut = 0x0;
  u32 uStatusReg = 0x0;

  ClearStatusRegisterCmd(uBlockAddress);

  UnlockBlockCmd(uBlockAddress & FLASH_BLOCK_ADDRESS_MASK);

  BlockEraseCmd(uBlockAddress & FLASH_BLOCK_ADDRESS_MASK);

  do
  {
    uStatusReg = ReadStatusRegisterCmd(uBlockAddress & FLASH_PARTITION_ADDRESS_MASK);
    Delay(100);
    uTimeOut ++;
  }
  while(((uStatusReg & FLASH_STATUS_WRITE_STATUS) == 0x0) && (uTimeOut < FLASH_ERASE_TIMEOUT));


  if (uTimeOut == FLASH_ERASE_TIMEOUT)
  {
    log_printf(LOG_LEVEL_INFO, "Erase timeout!\r\n");
    return XST_FAILURE;
  }

  if ((uStatusReg & FLASH_STATUS_ERASE_STATUS) != 0x0)
  {
    log_printf(LOG_LEVEL_INFO, "Erase error!\r\n");
    return XST_FAILURE;
  }

  if ((uStatusReg & FLASH_STATUS_VPP_STATUS) != 0x0)
  {
    log_printf(LOG_LEVEL_INFO, "VPP error!\r\n");
    return XST_FAILURE;
  }

  if ((uStatusReg & FLASH_STATUS_BLOCK_LOCK_ERROR) != 0x0)
  {
    log_printf(LOG_LEVEL_INFO, "Lock error!\r\n");
    return XST_FAILURE;
  }

  ClearStatusRegisterCmd(uBlockAddress);

  return XST_SUCCESS;

}

//=================================================================================
//  EnableDebugSdramReadMode
//--------------------------------------------------------------------------------
//  This method is used to enable or disable the SDRAM debug read mode.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uEnable   IN    1 = enable, 0 = disable
//
//  Return
//  ------
//  Two SDRAM 16-bit words output as single 32-bit word
//=================================================================================
void EnableDebugSdramReadMode(u8 uEnable)
{
  if (uEnable == 1)
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, DEBUG_SDRAM_READ_MODE);
  else
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);

}

//=================================================================================
//  ReadSdramWord
//--------------------------------------------------------------------------------
//  This method provides a debug option of asynchronously reading two 16-bit words
//  from the SDRAM. The SDRAM address is handled by the Spartan 3AN so no address
//  specified. The address will increment by two after this method.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  Two SDRAM 16-bit words output as single 32-bit word
//=================================================================================
u32 ReadSdramWord()
{
  return (Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR));
}

//=================================================================================
//  ContinuityTest
//--------------------------------------------------------------------------------
//  This method is used to do a basic continuity test of the flash interface between
//  the Virtex FPGA and the Spartan FPGA.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uOutput   OUT   Continuity data to output
//
//  Return
//  ------
//  Read continuity data
//=================================================================================
u32 ContinuityTest(u32 uOutput)
{
  u32 uReg;
  u32 uContinuityReg;

  // Output data
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_CONTINUITY_TEST_OUTPUT_REG, uOutput);

  // Check first lower 16 bits
  uReg = CONTINUITY_TEST_MODE | CONTINUITY_TEST_LOW_OUTPUT;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, uReg);

  Delay(100);

  uContinuityReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_CONTINUITY_TEST_OUTPUT_REG);

  // Next check upper 16 bits
  uReg = CONTINUITY_TEST_MODE | CONTINUITY_TEST_HIGH_OUTPUT;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, uReg);

  Delay(100);

  uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_CONTINUITY_TEST_OUTPUT_REG);

  // Disable continuity check now
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_SDRAM_CONFIG_IO_REG_ADDRESS, 0x0);
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + FLASH_SDRAM_SPI_ICAPE_ADDR + FLASH_CONTINUITY_TEST_OUTPUT_REG, 0x0);

  uContinuityReg = uContinuityReg | ((uReg & 0xFFFF) << 16);

  return uContinuityReg;
}


