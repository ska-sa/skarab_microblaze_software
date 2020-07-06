#include <xil_types.h>
#include <xstatus.h>

#include "init.h"
#include "logging.h"
#include "scratchpad.h"
#include "constant_defs.h"
#include "i2c_master.h"

u8 init_persistent_memory_setup(void){
  u8 pmem_status;

  pmem_status = PersistentMemory_Check();

  if (pmem_status == PMEM_RETURN_DEFAULT){
    /* if the memory in it's default state, we're safe to proceed and clear
     * the memory. This is an indication that 1) the FLASH location has not been
     * set for the board and following this 2) the board has been hard power
     * cycled. */
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] persistent memory contains default value. Clearing...\r\n");
    pmem_status = PersistentMemory_Clear();
  }

  /* catch any errors */
  if (pmem_status == PMEM_RETURN_ERROR){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] error setting up persistent memory.\r\n");
  }

  return pmem_status;
}



//=================================================================================
//  UpdateGBEPHYConfiguration
//--------------------------------------------------------------------------------
//  This method updates the configuration of the 1GBE PHY to improve the link
//  compatibility with different NICs. The drivers for some NICs do not support sleep 
//  reliably resulting in occasional packet loss. Also enable flow control through
//  pause frames to prevent possible packet loss in the Marvell 1GBE PHY. This is equivalent 
//  to setting DIS_SLEEP = '1' and ENA_PAUSE = '1'
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  None
//
//  Return
//  ------
//  None
//=================================================================================
void UpdateGBEPHYConfiguration()
{
  int iSuccess;
  u16 uWriteBytes[4];
  u16 uReadBytes[4];
  u16 uCurrentControlReg;

  // Set the switch to the GBE PHY
  uWriteBytes[0] = ONE_GBE_SWITCH_SELECT;
  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, uWriteBytes, 1);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to open I2C switch.\r\n");

  // Select PAGE 0
  uWriteBytes[0]  = 22; // Address of register to write
  uWriteBytes[1]  = 0;
  uWriteBytes[2]  = 0; // PAGE 0

  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uWriteBytes, 3);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to select PAGE 0.\r\n");

  // Update PHY SPECIFIC CONTROL REGISTER (16), ENERGY DETECT = "00" (DIS_SLEEP = '1')
  uWriteBytes[0] = 16; // Address of register to write
  uWriteBytes[1] = 0xF0;
  uWriteBytes[2] = 0x78;

  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uWriteBytes, 3);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to update PHY SPECIFIC CONTROL REG.\r\n");

  // Update AUTO NEGOTIATION ADVERTISEMENT REGISTER (4), support PAUSE (ENA_PAUSE = '1')
  uWriteBytes[0] = 4; // Address of register to write
  uWriteBytes[1] = 0x0D;
  uWriteBytes[2] = 0xE1;

  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uWriteBytes, 3);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to update AUTO NEGOTIATION ADVERTISEMENT REG.\r\n");

  // Read register 0 to get current configuration
  uWriteBytes[0] = 0; // Address of register to read

  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uWriteBytes, 1);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to update current read register.\r\n");

  iSuccess = ReadI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uReadBytes, 2);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to read CONTROL REG.\r\n");

  uCurrentControlReg = ((uReadBytes[0] << 8) | uReadBytes[1]);
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "1GBE [..] Current 1GBE PHY configuration: 0x%x.\r\n", uCurrentControlReg);

  // Trigger a soft reset of 1GBE PHY to update configuration
  // Do a soft reset
  uCurrentControlReg = uCurrentControlReg | 0x8000;

  uWriteBytes[0] = 0; // Address of register to write
  uWriteBytes[1] = ((uCurrentControlReg >> 8) & 0xFF);
  uWriteBytes[2] = (uCurrentControlReg & 0xFF);

  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, GBE_88E1111_I2C_DEVICE_ADDRESS, uWriteBytes, 3);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to write CONTROL REG.\r\n");

  // Close I2C switch
  uWriteBytes[0] = 0x0;
  iSuccess = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, uWriteBytes, 1);

  if (iSuccess == XST_FAILURE)
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "UpdateGBEPHYConfiguration: Failed to close I2C switch.\r\n");

}
