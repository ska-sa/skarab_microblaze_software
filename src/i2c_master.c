/**------------------------------------------------------------------------------
 *  FILE NAME            : i2c_master.c
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
 *  This file contains the implementation of functions to access the OC I2C
 *  master over the Wishbone bus.
 * ------------------------------------------------------------------------------*/

#include <xstatus.h>
#include <xil_io.h>

#include "i2c_master.h"
#include "constant_defs.h"
#include "register.h"
#include "delay.h"
#include "logging.h"
#include "custom_constants.h"

//=================================================================================
//  GetI2CAddressOffset
//--------------------------------------------------------------------------------
//  This method gets the offset address of the selected I2C master on the Wishbone bus.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId     IN    ID of I2C master want offset address for
//
//  Return
//  ------
//  Wishbone bus address offset
//=================================================================================
u32 GetI2CAddressOffset(u16 uId)
{
  if (uId == 0)
    return I2C_0_ADDR;
  else if (uId == 1)
    return I2C_1_ADDR;
  else if (uId == 2)
    return I2C_2_ADDR;
  else if (uId == 3)
    return I2C_3_ADDR;
  else
    return I2C_4_ADDR;

}

//=================================================================================
//  InitI2C
//--------------------------------------------------------------------------------
//  This method initialises the I2C clock prescaler and enables it.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId     IN    ID of I2C master want initialise
//  uSpeed    IN    Speed that want to initialise to
//
//  Return
//  ------
//  None
//=================================================================================
void InitI2C(u16 uId, u16 uSpeed)
{
  u32 uReg;
  u32 uAddressOffset = GetI2CAddressOffset(uId);

  if (uSpeed == SPEED_100kHz)
  {
    uReg = SPEED_100kHz_CLOCK_PRESCALER & 0xFF;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_PRER_LO * 4), uReg);

    uReg = (SPEED_100kHz_CLOCK_PRESCALER >> 8) & 0xFF;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_PRER_HI * 4), uReg);
  }
  else
  {
    uReg = SPEED_400kHz_CLOCK_PRESCALER & 0xFF;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_PRER_LO * 4), uReg);

    uReg = (SPEED_400kHz_CLOCK_PRESCALER >> 8) & 0xFF;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_PRER_HI * 4), uReg);
  }

  uReg = OC_I2C_EN;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CTR * 4), uReg);

}

//=================================================================================
//  WriteI2CBytes
//--------------------------------------------------------------------------------
//  This method writes a burst of bytes to I2C.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master want to write to
//  uSlaveAddress IN  I2C slave address of device want to access
//  uWriteBytes   IN  Buffer containing bytes to write
//  uNumBytes   IN  Number of bytes to write
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int WriteI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uWriteBytes, u16 uNumBytes)
{
  u32 uReg;
  u32 uAddressOffset = GetI2CAddressOffset(uId);
  u32 uTimeout = 0;
  u16 uByteCount;

  // USB PHY only has control over MB I2C
  if (uId == MB_I2C_BUS_ID)
  {
    // Check if the I2C bus is currently being used by USB PHY
    uReg = ReadBoardRegister(C_RD_USB_STAT_ADDR);

    if ((uReg & USB_I2C_CONTROL) != 0)
    {
      // USB PHY has control over I2C so return XST_FAILURE
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "WriteI2CBytes: USB PHY has control of I2C\r\n");
      return XST_FAILURE;
    }
  }

  // Write slave address
  uReg = uSlaveAddress << 1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "WriteI2CBytes: Timeout waiting for address write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "WriteI2CBytes: Address byte not ACKed\r\n");
    return XST_FAILURE;
  }

  // Write data
  for (uByteCount = 0; uByteCount < uNumBytes; uByteCount++)
  {
    uReg = uWriteBytes[uByteCount];
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

    if ((uByteCount + 1) == uNumBytes)
      uReg = OC_I2C_WR | OC_I2C_STO;
    else
      uReg = OC_I2C_WR;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

    // Wait for transaction to complete
    uTimeout = 0x0;
    do
    {
      uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
      Delay(1);
      uTimeout++;
    }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

    if (uTimeout == I2C_TIMEOUT)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "WriteI2CBytes: Timeout waiting for data byte write to complete\r\n");
      return XST_FAILURE;
    }

    // Check the received ACK, should be '0'
    if ((uReg & OC_I2C_RXACK) != 0x0)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "WriteI2CBytes: Data byte not ACKed\r\n");
      return XST_FAILURE;
    }

  }

  return XST_SUCCESS;

}

//=================================================================================
//  ReadI2CBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of bytes from I2C.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master want to read from
//  uSlaveAddress IN  I2C slave address of device want to access
//  uReadBytes    OUT Buffer to store bytes read
//  uNumBytes   IN  Number of bytes to read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int ReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uReadBytes, u16 uNumBytes)
{
  u32 uReg;
  u32 uAddressOffset = GetI2CAddressOffset(uId);
  u32 uTimeout = 0;
  u16 uByteCount;

  // USB PHY only has control over MB I2C
  if (uId == MB_I2C_BUS_ID)
  {

    // Check if the I2C bus is currently being used by USB PHY
    uReg = ReadBoardRegister(C_RD_USB_STAT_ADDR);

    if ((uReg & USB_I2C_CONTROL) != 0)
    {
      // USB PHY has control over I2C so return XST_FAILURE
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ReadI2CBytes: USB PHY has control of I2C\r\n");
      return XST_FAILURE;
    }
  }

  // Write slave address, set read bit
  uReg = (uSlaveAddress << 1) | 0x1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ReadI2CBytes: Timeout waiting for address write to complete.\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ReadI2CBytes: Address not ACKed.\r\n");
    return XST_FAILURE;
  }

  for (uByteCount = 0; uByteCount < uNumBytes; uByteCount++)
  {
    if ((uByteCount + 1) == uNumBytes)
      // For last byte, must set stop bit and NACK
      uReg = OC_I2C_RD | OC_I2C_STO | OC_I2C_ACK;
    else
      uReg = OC_I2C_RD;

    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);


    // Wait for transaction to complete
    do
    {
      uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
      Delay(1);
      uTimeout++;
    }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

    if (uTimeout == I2C_TIMEOUT)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ReadI2CBytes: Timeout waiting for read data bytes to complete.\r\n");
      return XST_FAILURE;
    }

    // Get read bytes
    uReadBytes[uByteCount] = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_RXR * 4));
  }

  return XST_SUCCESS;

}

//=================================================================================
//  PMBusReadI2CBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of bytes from I2C. The PMBus format is followed which
//  requires first writing the command, then doing a repeated start followed by the
//  reads. PMBus compliant writes can be achieved using the standard WriteI2CBytes.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master want to read from
//  uSlaveAddress IN  I2C slave address of device want to access
//  uCommandCode  IN  PMBus command to execute
//  uReadBytes    OUT Buffer to store bytes read
//  uNumBytes   IN  Number of bytes to read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int PMBusReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 uCommandCode, u16 * uReadBytes, u16 uNumBytes)
{
  u32 uReg;
  u32 uAddressOffset = GetI2CAddressOffset(uId);
  u32 uTimeout = 0;
  u16 uByteCount;

  if (MAX31785_I2C_DEVICE_ADDRESS == uSlaveAddress){
    Delay(1400);
  }

  // USB PHY only has control over MB I2C
  if (uId == MB_I2C_BUS_ID)
  {
    // Check if the I2C bus is currently being used by USB PHY
    uReg = ReadBoardRegister(C_RD_USB_STAT_ADDR);

    if ((uReg & USB_I2C_CONTROL) != 0)
    {
      // USB PHY has control over I2C so return XST_FAILURE
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: USB PHY has control of I2C\r\n");
      return XST_FAILURE;
    }
  }

  // Write slave address
  uReg = uSlaveAddress << 1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Timeout waiting for first address write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: First Address write not ACKed\r\n");
    return XST_FAILURE;
  }

  // Write command
  uReg = uCommandCode;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  uReg = OC_I2C_WR;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  uTimeout = 0x0;
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Timeout waiting for command write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Command write not ACKed\r\n");
    return XST_FAILURE;
  }

  // Write slave address, set read bit
  uReg = (uSlaveAddress << 1) | 0x1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start (repeated start because was no stop)
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Timeout waiting for second address write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Second address write not ACKed\r\n");
    return XST_FAILURE;
  }

  for (uByteCount = 0; uByteCount < uNumBytes; uByteCount++)
  {
    if ((uByteCount + 1) == uNumBytes)
      // For last byte, must set stop bit and NACK
      uReg = OC_I2C_RD | OC_I2C_STO | OC_I2C_ACK;
    else
      uReg = OC_I2C_RD;

    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);


    // Wait for transaction to complete
    do
    {
      uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
      /* Delay(1); */
      /*
       * NOTE: this delay limits the amount of successive bytes that can be read.
       * Through experimentation, it was observed that a delay of 10000us or 10ms
       * allows for about 135 bytes to be read. TODO: this function could be made to
       * determine the delay dynamically depending on the number of bytes to be read.
       */
      Delay(3);
      uTimeout++;
    }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

    if (uTimeout == I2C_TIMEOUT)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "PMBusReadI2CBytes: Timeout waiting for read data byte\r\n");
      return XST_FAILURE;
    }

    // Get read bytes
    uReadBytes[uByteCount] = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_RXR * 4));
  }

  return XST_SUCCESS;

}

//=================================================================================
//  HMCReadI2CBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of four bytes from HMC I2C. The HMC requires a repeated
//  start when doing a read.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master want to read from
//  uSlaveAddress IN  I2C slave address of device want to access
//  uReadAddress  IN  Register address want to read
//  uReadBytes    OUT Buffer to store bytes read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int HMCReadI2CBytes(u16 uId, u16 uSlaveAddress, u16 * uReadAddress, u16 * uReadBytes)
{
  u32 uReg;
  u32 uAddressOffset = GetI2CAddressOffset(uId);
  u32 uTimeout = 0;
  u16 uByteCount;

  // USB PHY only has control over MB I2C
  if (uId == MB_I2C_BUS_ID)
  {
    // Check if the I2C bus is currently being used by USB PHY
    uReg = ReadBoardRegister(C_RD_USB_STAT_ADDR);

    if ((uReg & USB_I2C_CONTROL) != 0)
    {
      // USB PHY has control over I2C so return XST_FAILURE
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: USB PHY has control of I2C\r\n");
      return XST_FAILURE;
    }
  }

  // Write slave address
  uReg = uSlaveAddress << 1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Timeout waiting for first address write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: First Address write not ACKed\r\n");
    return XST_FAILURE;
  }

  // Write address bytes
  for (uByteCount = 0; uByteCount < 4; uByteCount++)
  {
    uReg = uReadAddress[uByteCount];
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

    uReg = OC_I2C_WR;
    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

    // Wait for transaction to complete
    uTimeout = 0x0;
    do
    {
      uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
      Delay(1);
      uTimeout++;
    }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

    if (uTimeout == I2C_TIMEOUT)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Timeout waiting for address write to complete\r\n");
      return XST_FAILURE;
    }

    // Check the received ACK, should be '0'
    if ((uReg & OC_I2C_RXACK) != 0x0)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Address write not ACKed\r\n");
      return XST_FAILURE;
    }
  }

  // Write slave address, set read bit
  uReg = (uSlaveAddress << 1) | 0x1;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_TXR * 4), uReg);

  // Generate write start (repeated start because was no stop)
  uReg = OC_I2C_WR | OC_I2C_STA;
  Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);

  // Wait for transaction to complete
  do
  {
    uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
    Delay(1);
    uTimeout++;
  }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

  if (uTimeout == I2C_TIMEOUT)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Timeout waiting for second address write to complete\r\n");
    return XST_FAILURE;
  }

  // Check the received ACK, should be '0'
  if ((uReg & OC_I2C_RXACK) != 0x0)
  {
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Second address write not ACKed\r\n");
    return XST_FAILURE;
  }

  for (uByteCount = 0; uByteCount < 4; uByteCount++)
  {
    if ((uByteCount + 1) == 4)
      // For last byte, must set stop bit and NACK
      uReg = OC_I2C_RD | OC_I2C_STO | OC_I2C_ACK;
    else
      uReg = OC_I2C_RD;

    Xil_Out32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_CR * 4), uReg);


    // Wait for transaction to complete
    do
    {
      uReg = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_SR * 4));
      Delay(1);
      uTimeout++;
    }while(((uReg & OC_I2C_TIP) != 0x0)&&(uTimeout < I2C_TIMEOUT));

    if (uTimeout == I2C_TIMEOUT)
    {
      log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "HMCReadI2CBytes: Timeout waiting for read data byte\r\n");
      return XST_FAILURE;
    }

    // Get read bytes
    uReadBytes[uByteCount] = Xil_In32(XPAR_AXI_SLAVE_WISHBONE_CLASSIC_MASTER_0_BASEADDR + uAddressOffset + (OC_I2C_RXR * 4));
  }

  return XST_SUCCESS;

}

//=================================================================================
//  HMCWriteI2CBytes
//--------------------------------------------------------------------------------
//  This method writes a 32-bit word to HMC registers over the I2C.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId             IN  ID of I2C master
//  uSlaveAddress   IN  I2C slave address of device to be accessed
//  uWriteAddress   IN  32-bit register address
//  uWriteBytes     IN  32-bit data word
//
//  Return
//  ------
//  XST_SUCCESS or XST_FAILURE
//=================================================================================
int HMCWriteI2CBytes(u16 uId, u16 uSlaveAddress, u32 uWriteAddress, u32 uWriteData){
  /* TODO: check if the card in that mezz slot (given by uId) is actually an HMC */

  u16 uWriteBytes[8];

  /* write the following 8 bytes of data to the I2C bus */
  uWriteBytes[0] = (uWriteAddress >> 24) & 0xff;     /* MSB of HMC reg addr */
  uWriteBytes[1] = (uWriteAddress >> 16) & 0xff;
  uWriteBytes[2] = (uWriteAddress >>  8) & 0xff;
  uWriteBytes[3] = (uWriteAddress      ) & 0xff;     /* LSB of HMC reg addr */

  uWriteBytes[4] = (uWriteData >> 24) & 0xff;        /* MSB of data word to be written */
  uWriteBytes[5] = (uWriteData >> 16) & 0xff;
  uWriteBytes[6] = (uWriteData >>  8) & 0xff;
  uWriteBytes[7] = (uWriteData      ) & 0xff;        /* LSB of data word to be written */

  return WriteI2CBytes(uId, uSlaveAddress, uWriteBytes, 8);
}

//=================================================================================
//  I2CPCA9548SelectChannel
//--------------------------------------------------------------------------------
//  This method selects a specific channel of PCA9548.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId         IN  ID of I2C master
//  uChannelSelection IN  Channel selection
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CPCA9548SelectChannel(u16 uId, u16 uChannelSelection)
{
  u16 uSlaveAddress;
  u16 uWriteBytes[1];

  if (uId == 0)
    uSlaveAddress = PCA9548_0_ADDRESS;
  else if (uId == 1)
    uSlaveAddress = PCA9548_1_ADDRESS;
  else
    uSlaveAddress = PCA9548_2_ADDRESS;

  uWriteBytes[0] = uChannelSelection;

  return (WriteI2CBytes(uId, uSlaveAddress, uWriteBytes, 0x1));

}

//=================================================================================
//  I2CReadFMCClkOscBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of bytes from the selected clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master
//  uByteAddress  IN  Starting byte address
//  uReadBytes    OUT Bytes read
//  uNumBytes   IN  Number of bytes to read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CFMCReadClkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes)
{
  int iStatus;
  u16 uWriteBytes[2];

  uWriteBytes[0] = uByteAddress;

  iStatus = I2CPCA9548SelectChannel(uId, PCA9548_FMC_OSC);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = WriteI2CBytes(uId,SI570_FMC_OSC_ADDRESS, uWriteBytes, 1);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = ReadI2CBytes(uId, SI570_FMC_OSC_ADDRESS, uReadBytes, uNumBytes);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CReadSFPClkOscBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of bytes from the selected clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master
//  uByteAddress  IN  Starting byte address
//  uReadBytes    OUT Bytes read
//  uNumBytes   IN  Number of bytes to read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CSFPReadClkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes)
{
  int iStatus;
  u16 uWriteBytes[2];

  uWriteBytes[0] = uByteAddress;

  iStatus = I2CPCA9548SelectChannel(uId, PCA9548_SFP_OSC);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = WriteI2CBytes(uId,SI570_SFP_OSC_ADDRESS, uWriteBytes, 1);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = ReadI2CBytes(uId, SI570_SFP_OSC_ADDRESS, uReadBytes, uNumBytes);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CReadRefclkOscBytes
//--------------------------------------------------------------------------------
//  This method reads a burst of bytes from the selected clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master
//  uByteAddress  IN  Starting byte address
//  uReadBytes    OUT Bytes read
//  uNumBytes   IN  Number of bytes to read
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CReadRefclkOscBytes(u16 uId, u16 uByteAddress, u16* uReadBytes, unsigned uNumBytes)
{
  int iStatus;
  u16 uWriteBytes[2];

  uWriteBytes[0] = uByteAddress;

  iStatus = I2CPCA9548SelectChannel(uId, PCA9548_REFCLK_OSC);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = WriteI2CBytes(uId,SI570_REFCLK_OSC_ADDRESS, uWriteBytes, 1);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  iStatus = ReadI2CBytes(uId, SI570_REFCLK_OSC_ADDRESS, uReadBytes, uNumBytes);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CSFPWriteClkOscBytes
//--------------------------------------------------------------------------------
//  This method write a burst of bytes to the selected clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master
//  uByteAddress  IN  Starting byte address
//  uReadBytes    IN  Bytes to write
//  uNumBytes   IN  Number of bytes to write
//  uOpenSwitch   IN  Open switch first or not
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CSFPWriteClkOscBytes(u16 uId, u16 uByteAddress, u16* uWriteBytes, unsigned uNumBytes, u16 uOpenSwitch)
{
  int iStatus;
  u16 uByteCopy;
  u16 uWriteAddressBytes[8];

  uWriteAddressBytes[0] = uByteAddress;
  for (uByteCopy = 0; uByteCopy < uNumBytes; uByteCopy++)
  {
    uWriteAddressBytes[1 + uByteCopy] = uWriteBytes[uByteCopy];
  }

  if (uOpenSwitch)
  {
    iStatus = I2CPCA9548SelectChannel(uId, PCA9548_SFP_OSC);

    if (iStatus == XST_FAILURE)
      return XST_FAILURE;
  }

  iStatus = WriteI2CBytes(uId, SI570_SFP_OSC_ADDRESS, uWriteAddressBytes, (uNumBytes + 1));

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CWriteRefclkOscBytes
//--------------------------------------------------------------------------------
//  This method write a burst of bytes to the selected clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  uId       IN  ID of I2C master
//  uByteAddress  IN  Starting byte address
//  uReadBytes    IN  Bytes to write
//  uNumBytes   IN  Number of bytes to write
//  uOpenSwitch   IN  Open switch first or not
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CWriteRefclkOscBytes(u16 uId, u16 uByteAddress, u16* uWriteBytes, unsigned uNumBytes, u16 uOpenSwitch)
{
  int iStatus;
  u16 uByteCopy;
  u16 uWriteAddressBytes[8];

  uWriteAddressBytes[0] = uByteAddress;
  for (uByteCopy = 0; uByteCopy < uNumBytes; uByteCopy++)
  {
    uWriteAddressBytes[1 + uByteCopy] = uWriteBytes[uByteCopy];
  }

  if (uOpenSwitch)
  {
    iStatus = I2CPCA9548SelectChannel(uId, PCA9548_REFCLK_OSC);

    if (iStatus == XST_FAILURE)
      return XST_FAILURE;
  }

  iStatus = WriteI2CBytes(uId, SI570_REFCLK_OSC_ADDRESS, uWriteAddressBytes, (uNumBytes + 1));

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CProgramSFPClkOsc
//--------------------------------------------------------------------------------
//  This method programs the SFP clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CProgramSFPClkOsc()
{
  int iStatus;
  u16 uWriteBytes[3];

  // FREEZE DCO
  uWriteBytes[0] = SI570_FREEZE_DCO_VAL;

  iStatus = I2CSFPWriteClkOscBytes(0, SI570_FREEZE_DCO_ADDRESS, uWriteBytes, 1, 1);


  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // UPDATE REQUIRED REGISTERS
  uWriteBytes[0] = SI570_SPF_REG_7_VAL;
  uWriteBytes[1] = SI570_SFP_REG_8_VAL;

  iStatus = I2CSFPWriteClkOscBytes(0, SI570_HS_N1_ADDRESS, uWriteBytes, 2, 0);


  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // UNFREEZE DCO
  uWriteBytes[0] = 0x0;

  iStatus = I2CSFPWriteClkOscBytes(0, SI570_FREEZE_DCO_ADDRESS, uWriteBytes, 1, 0);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // SET NEW FREQ BIT
  uWriteBytes[0] = SI570_NEW_FREQ_VAL;

  iStatus = I2CSFPWriteClkOscBytes(0, SI570_NEW_FREQ_ADDRESS, uWriteBytes, 1, 0);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}

//=================================================================================
//  I2CProgramRefClkOsc
//--------------------------------------------------------------------------------
//  This method programs the Ref clock oscillator.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int I2CProgramRefClkOsc()
{
  int iStatus;
  u16 uWriteBytes[3];

  // FREEZE DCO
  uWriteBytes[0] = SI570_FREEZE_DCO_VAL;

  iStatus = I2CWriteRefclkOscBytes(2, SI570_FREEZE_DCO_ADDRESS, uWriteBytes, 1, 1);


  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // UPDATE REQUIRED REGISTERS
  uWriteBytes[0] = SI570_REFCLK_REG_7_VAL;
  uWriteBytes[1] = SI570_REFCLK_REG_8_VAL;

  iStatus = I2CWriteRefclkOscBytes(2, SI570_HS_N1_ADDRESS, uWriteBytes, 2, 0);


  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // UNFREEZE DCO
  uWriteBytes[0] = 0x0;

  iStatus = I2CWriteRefclkOscBytes(2, SI570_FREEZE_DCO_ADDRESS, uWriteBytes, 1, 0);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  // SET NEW FREQ BIT
  uWriteBytes[0] = SI570_NEW_FREQ_VAL;

  iStatus = I2CWriteRefclkOscBytes(2, SI570_NEW_FREQ_ADDRESS, uWriteBytes, 1, 0);

  if (iStatus == XST_FAILURE)
    return XST_FAILURE;

  return XST_SUCCESS;
}
