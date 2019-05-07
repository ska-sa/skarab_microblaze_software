#include <xil_types.h>
#include <xstatus.h>

#include "qsfp.h"
#include "constant_defs.h"
#include "i2c_master.h"
#include "logging.h"
#include "register.h"
#include "delay.h"

/* Local function prototypes  */
//static typeQSFPInitState qsfp_reset_state(struct sQSFPObject *pQSFPObject);
static typeQSFPInitState qsfp_init_bootloader_version_wr_state(struct sQSFPObject *pQSFPObject);
static typeQSFPInitState qsfp_init_bootloader_version_rd_state(struct sQSFPObject *pQSFPObject);
static typeQSFPInitState qsfp_init_bootloader_state(struct sQSFPObject *pQSFPObject);
static typeQSFPInitState qsfp_init_starting_application_state(struct sQSFPObject *pQSFPObject);
static typeQSFPInitState qsfp_init_application_state(struct sQSFPObject *pQSFPObject);
//static typeQSFPInitState qsfp_init_bootloader_program_state(struct sQSFPObject *pQSFPObject);

static typeQSFPAppState qsfp_app_update_tx_leds(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_rx_leds(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_0_wr(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_0_rd(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_1_wr(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_1_rd(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_2_wr(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_2_rd(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_3_wr(struct sQSFPObject *pQSFPObject);
static typeQSFPAppState qsfp_app_update_mod_prsnt_3_rd(struct sQSFPObject *pQSFPObject);

typedef typeQSFPInitState (*qsfp_init_state_func_ptr)(struct sQSFPObject *pQSFPObject);
typedef typeQSFPAppState (*qsfp_app_state_func_ptr)(struct sQSFPObject *pQSFPObject);

static qsfp_init_state_func_ptr qsfp_init_state_table[] = {
  //[QSFP_STATE_INIT_RESET]                         = qsfp_reset_state,
  [QSFP_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE] = qsfp_init_bootloader_version_wr_state,
  [QSFP_STATE_INIT_BOOTLOADER_VERSION_READ_MODE]  = qsfp_init_bootloader_version_rd_state,
  [QSFP_STATE_INIT_BOOTLOADER_MODE]               = qsfp_init_bootloader_state,
  [QSFP_STATE_INIT_STARTING_APPLICATION_MODE]     = qsfp_init_starting_application_state,
  [QSFP_STATE_INIT_APPLICATION_MODE]              = qsfp_init_application_state
  //[QSFP_STATE_INIT_BOOTLOADER_PROGRAMMING_MODE]   = qsfp_init_bootloader_program_state
};

static qsfp_app_state_func_ptr qsfp_app_state_table[] = {
  [QSFP_STATE_APP_UPDATING_TX_LEDS]         = qsfp_app_update_tx_leds,
  [QSFP_STATE_APP_UPDATING_RX_LEDS]         = qsfp_app_update_rx_leds,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_WR]  = qsfp_app_update_mod_prsnt_0_wr,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_RD]  = qsfp_app_update_mod_prsnt_0_rd,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_WR]  = qsfp_app_update_mod_prsnt_1_wr,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_RD]  = qsfp_app_update_mod_prsnt_1_rd,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_WR]  = qsfp_app_update_mod_prsnt_2_wr,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_RD]  = qsfp_app_update_mod_prsnt_2_rd,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_WR]  = qsfp_app_update_mod_prsnt_3_wr,
  [QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_RD]  = qsfp_app_update_mod_prsnt_3_rd
};

/* 
 * Notes:
 * TODO FIXME
 * narrow down global variable scope if possible.
 */

/* these global variables are an artefact of the original code where the state
 * variables where accessible globally throughout the codebase. With the move to
 * a more contained state machine, there is still a need to reach into the state
 * machine and reset it from outside the context of the QSFP Object. TODO in
 * future one would link the QSFP Object to the IF Object and pass that around. 
 * FIXME currently, codebase only supports one qsfp module and thus this
 * solution would not work for more qsfp modules since this variable carries no
 * information about which one to reset.
 */
static volatile u8 qsfp_init_reset_global = 0;  /* reset of the complete state machine */
static volatile u8 qsfp_app_reset_global = 0;   /* reset of the application state machine */
static volatile u8 qsfp_sm_pause_global = 0;    /* flag to pause (1) the state
                                                   machine.  This is required
                                                   when the bootloader on the
                                                   STM is being programmed -
                                                   similar to the previous
                                                   bootloader_programming_mode
                                                   state. */

u8 uQSFPInit(struct sQSFPObject *pQSFPObject){
  if (pQSFPObject == NULL){
    return XST_FAILURE;
  }

  pQSFPObject->uWaitCount = 0;
  pQSFPObject->InitState = QSFP_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE;
  pQSFPObject->AppState = QSFP_STATE_APP_UPDATING_TX_LEDS;

  return XST_SUCCESS;
}


u8 QSFPStateMachine(struct sQSFPObject *pQSFPObject){
  if (pQSFPObject == NULL){
    return XST_FAILURE;
  }

  if (qsfp_sm_pause_global == 0){
    pQSFPObject->InitState = qsfp_init_state_table[pQSFPObject->InitState](pQSFPObject);
  }

  return XST_SUCCESS;
}

/* wrapper functions for internal global states */
void uQSFPResetInit(void){
  qsfp_init_reset_global = 1;
  /* also reset the application state */
  qsfp_app_reset_global = 1;
}

void uQSFPResetApp(void){
  qsfp_app_reset_global = 1;
}

void uQSFPStateMachineResume(void){
  qsfp_sm_pause_global = 0;
}

void uQSFPStateMachinePause(void){
  qsfp_sm_pause_global = 1;
}

static typeQSFPInitState qsfp_init_bootloader_version_wr_state(struct sQSFPObject *pQSFPObject){
  u16 uWriteBytes[6];
  int iStatus;

  // Need to read and store QSFP+ bootloader version before exit bootloader mode
  uWriteBytes[0] = QSFP_BOOTLOADER_READ_OPCODE;
  uWriteBytes[1] = ((QSFP_BOOTLOADER_VERSION_ADDRESS >> 24) & 0xFF);
  uWriteBytes[2] = ((QSFP_BOOTLOADER_VERSION_ADDRESS >> 16) & 0xFF);
  uWriteBytes[3] = ((QSFP_BOOTLOADER_VERSION_ADDRESS >> 8) & 0xFF);
  uWriteBytes[4] = (QSFP_BOOTLOADER_VERSION_ADDRESS & 0xFF);
  uWriteBytes[5] = 0x01; // Reading 1 byte

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uWriteBytes, 6);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] Bootloader I2C write [FAILED]\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_INIT_BOOTLOADER_VERSION_READ_MODE;
}


static typeQSFPInitState qsfp_init_bootloader_version_rd_state(struct sQSFPObject *pQSFPObject){
  u16 uReadBytes[1] = {0xffff};
  int iStatus;

  iStatus = ReadI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] Bootloader I2C read [FAILED]\r\n", uQSFPMezzanineLocation);
  }

  uQSFPBootloaderVersionMajor = (uReadBytes[0] >> 4) & 0xF;
  uQSFPBootloaderVersionMinor = uReadBytes[0] & 0xF;

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Mezzanine bootloader version: %x.%x\r\n", uQSFPMezzanineLocation, uQSFPBootloaderVersionMajor, uQSFPBootloaderVersionMinor);

  return QSFP_STATE_INIT_BOOTLOADER_MODE;
}


static typeQSFPInitState qsfp_init_bootloader_state(struct sQSFPObject *pQSFPObject){
  u16 uWriteBytes[1] = {0};
  int iStatus;

  uWriteBytes[0] = QSFP_LEAVE_BOOTLOADER_MODE;
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "QSFP+[%02x] Mezzanine leaving bootloader mode.\r\n", uQSFPMezzanineLocation);

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] Bootloader I2C write [FAILED]\r\n", uQSFPMezzanineLocation);
  }

  pQSFPObject->uWaitCount = 0;

  return QSFP_STATE_INIT_STARTING_APPLICATION_MODE;
}

#define QSFP_STARTING_TIMEOUT 30    /* x100 ms */
static typeQSFPInitState qsfp_init_starting_application_state(struct sQSFPObject *pQSFPObject){
  /* wait state - wait for QSFP/STM to be ready */
  if (pQSFPObject->uWaitCount >= QSFP_STARTING_TIMEOUT) {
    return QSFP_STATE_INIT_APPLICATION_MODE;
  } else {
    pQSFPObject->uWaitCount++;
    return QSFP_STATE_INIT_STARTING_APPLICATION_MODE;
  }
}

static typeQSFPInitState qsfp_init_application_state(struct sQSFPObject *pQSFPObject){
  /* the order of the next conditions are important since the state machine has
   * a 2-level hierarchy. */
  if (qsfp_init_reset_global == 1){
    /* 
     * reset the current qsfp context's init state
     */
    qsfp_init_reset_global = 0;
    return QSFP_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE;
    //return QSFP_STATE_INIT_BOOTLOADER_MODE;
  } else if (qsfp_app_reset_global == 1){
    /* reset the current qsfp context's application state */
    qsfp_app_reset_global = 0;
    pQSFPObject->AppState = QSFP_STATE_APP_UPDATING_TX_LEDS;
  } else if (uQSFPI2CMicroblazeAccess == QSFP_I2C_MICROBLAZE_ENABLE) {
    /* run next application state if we have access to i2c bus */
    pQSFPObject->AppState = qsfp_app_state_table[pQSFPObject->AppState](pQSFPObject);
  }
  return QSFP_STATE_INIT_APPLICATION_MODE;
}


static typeQSFPAppState qsfp_app_update_tx_leds(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u8 uId = 0x0;
  u16 uWriteBytes[2];
  u32 uReg;
  u32 uLinkMask = 0x10000;
  u32 uActivityMask = 0x20000;
  u32 uLedTxReg = 0x0;

  uReg = ReadBoardRegister(C_RD_ETH_IF_LINK_UP_ADDR);

  for (uId = 0x0; uId < (NUM_ETHERNET_INTERFACES - 1); uId++) {
    /* Check TX link */
    if ((uReg & uLinkMask) != 0x0) {
      /* Check if activity as well */
      if ((uReg & uActivityMask) != 0x0) {
        uLedTxReg = uLedTxReg | (LED_FLASHING << (uId * 2));
      } else {
        uLedTxReg = uLedTxReg | (LED_ON << (uId * 2));
      }
    } else {
      uLedTxReg = uLedTxReg | (LED_OFF << (uId * 2));
    }
    /* shift to next 40gbe tx pair */
    uLinkMask = uLinkMask << 4;
    uActivityMask = uActivityMask << 4;
  }

  uWriteBytes[0] = QSFP_LED_TX_REG_ADDRESS;
  uWriteBytes[1] = uLedTxReg;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 2);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] TX LED MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_RX_LEDS;
}


static typeQSFPAppState qsfp_app_update_rx_leds(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u8 uId = 0x0;
  u16 uWriteBytes[2];
  u32 uReg;
  u32 uLinkMask = 0x40000;
  u32 uActivityMask = 0x80000;
  u32 uLedRxReg = 0x0;

  uReg = ReadBoardRegister(C_RD_ETH_IF_LINK_UP_ADDR);

  for (uId = 0x0; uId < (NUM_ETHERNET_INTERFACES - 1); uId++) {
    /* Check RX link */
    if ((uReg & uLinkMask) != 0x0) {
      /* Check if activity as well */
      if ((uReg & uActivityMask) != 0x0) {
        uLedRxReg = uLedRxReg | (LED_FLASHING << (uId * 2));
      } else {
        uLedRxReg = uLedRxReg | (LED_ON << (uId * 2));
      }
    } else {
      uLedRxReg = uLedRxReg | (LED_OFF << (uId * 2));
    }

    /* shift to next 40gbe rx pair */
    uLinkMask = uLinkMask << 4;
    uActivityMask = uActivityMask << 4;
  }

  uWriteBytes[0] = QSFP_LED_RX_REG_ADDRESS;
  uWriteBytes[1] = uLedRxReg;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 2);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] RX LED MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_WR;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_0_wr(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uWriteBytes[1];

  uWriteBytes[0] = QSFP_MODULE_0_PRESENT_REG_ADDRESS;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 0 MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_RD;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_0_rd(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uReadBytes[1];

  iStatus = ReadI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 0 MEZ I2C READ FAILED\r\n", uQSFPMezzanineLocation);
  } else {
    if (uReadBytes[0] != 0x0) {
      /* If a module is present, take out of reset */
      uQSFPCtrlReg = uQSFPCtrlReg & (~QSFP0_RESET);  /* FIXME: try to remove global scope of uQSFPCtrlReg! */
    } else {
      /* If a module is not present, put in reset */
      uQSFPCtrlReg = uQSFPCtrlReg | QSFP0_RESET;
    }
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_WR;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_1_wr(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uWriteBytes[1];

  uWriteBytes[0] = QSFP_MODULE_1_PRESENT_REG_ADDRESS;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 1 MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_RD;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_1_rd(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uReadBytes[1];

  iStatus = ReadI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 1 MEZ I2C READ FAILED\r\n", uQSFPMezzanineLocation);
  } else {
    if (uReadBytes[0] != 0x0) {
      /* If a module is present, take out of reset */
      uQSFPCtrlReg = uQSFPCtrlReg & (~ QSFP1_RESET);
    } else {
      /* If a module is not present, put in reset */
      uQSFPCtrlReg = uQSFPCtrlReg | QSFP1_RESET;
    }
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_WR;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_2_wr(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uWriteBytes[1];

  uWriteBytes[0] = QSFP_MODULE_2_PRESENT_REG_ADDRESS;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 2 MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_RD;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_2_rd(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uReadBytes[1];

  iStatus = ReadI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 2 MEZ I2C READ FAILED\r\n", uQSFPMezzanineLocation);
  } else {
    if (uReadBytes[0] != 0x0) {
      /* If a module is present, take out of reset */
      uQSFPCtrlReg = uQSFPCtrlReg & (~ QSFP2_RESET);
    } else {
      /* If a module is not present, put in reset */
      uQSFPCtrlReg = uQSFPCtrlReg | QSFP2_RESET;
    }
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_WR;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_3_wr(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uWriteBytes[1];

  uWriteBytes[0] = QSFP_MODULE_3_PRESENT_REG_ADDRESS;

  iStatus = WriteI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 3 MEZ I2C WRITE FAILED\r\n", uQSFPMezzanineLocation);
  }

  return QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_RD;
}


static typeQSFPAppState qsfp_app_update_mod_prsnt_3_rd(struct sQSFPObject *pQSFPObject){
  int iStatus;
  u16 uReadBytes[1];

  iStatus = ReadI2CBytes(uQSFPMezzanineLocation + 1, QSFP_STM_I2C_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS) {
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "QSFP+[%02x] MOD 3 MEZ I2C READ FAILED\r\n", uQSFPMezzanineLocation);
  } else {
    if (uReadBytes[0] != 0x0) {
      /* If a module is present, take out of reset */
      uQSFPCtrlReg = uQSFPCtrlReg & (~ QSFP3_RESET);
    } else {
      /* If a module is not present, put in reset */
      uQSFPCtrlReg = uQSFPCtrlReg | QSFP3_RESET;
    }
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);
  }

  return QSFP_STATE_APP_UPDATING_TX_LEDS;
}

#if 0
static typeQSFPInitState qsfp_init_bootloader_program_state(struct sQSFPObject *pQSFPObject){
  /* 
   * the microblaze doesn't do anything in this state, it just waits around
   * until the state machine is reset after the bootloader has been updated.
   */
}
#endif
