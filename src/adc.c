/* Support for SKARAB ADC */

#include <xil_types.h>
#include <xstatus.h>

#include "constant_defs.h"
#include "adc.h"
#include "logging.h"
#include "i2c_master.h"

static typeAdcInitState adc_init_bootloader_version_wr_state(struct sAdcObject *pAdcObject);
static typeAdcInitState adc_init_bootloader_version_rd_state(struct sAdcObject *pAdcObject);
static typeAdcInitState adc_init_bootloader_state(struct sAdcObject *pAdcObject);
static typeAdcInitState adc_init_starting_application_state(struct sAdcObject *pAdcObject);
static typeAdcInitState adc_init_application_state(struct sAdcObject *pAdcObject);

static typeAdcAppState adc_app_do_nothing(struct sAdcObject *pAdcObject);

typedef typeAdcInitState (*adc_init_state_func_ptr)(struct sAdcObject *pAdcObject);
typedef typeAdcAppState (*adc_app_state_func_ptr)(struct sAdcObject *pAdcObject);

static adc_init_state_func_ptr adc_init_state_table[] = {
  [ADC_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE] = adc_init_bootloader_version_wr_state,
  [ADC_STATE_INIT_BOOTLOADER_VERSION_READ_MODE]  = adc_init_bootloader_version_rd_state,
  [ADC_STATE_INIT_BOOTLOADER_MODE]               = adc_init_bootloader_state,
  [ADC_STATE_INIT_STARTING_APPLICATION_MODE]     = adc_init_starting_application_state,
  [ADC_STATE_INIT_APPLICATION_MODE]              = adc_init_application_state
  //[ADC_STATE_INIT_BOOTLOADER_PROGRAMMING_MODE]   = adc_init_bootloader_program_state
};

static adc_app_state_func_ptr adc_app_state_table[] = {
  [ADC_STATE_APP_DO_NOTHING] = adc_app_do_nothing
    /* expand state functions in future if necessary */
};

/********** API functions **********/

u8 AdcInit(struct sAdcObject *pAdcObject){
  if (pAdcObject == NULL){
    return XST_FAILURE;
  }

  pAdcObject->uWaitCount = 0;
  pAdcObject->InitState = ADC_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE;
  pAdcObject->AppState = ADC_STATE_APP_DO_NOTHING;

  return XST_SUCCESS;
}

u8 AdcStateMachine(struct sAdcObject *pAdcObject){
  if (pAdcObject == NULL){
    return XST_FAILURE;
  }

  pAdcObject->InitState = adc_init_state_table[pAdcObject->InitState](pAdcObject);

  return XST_SUCCESS;
}


/********** Init state functions **********/

static typeAdcInitState adc_init_bootloader_version_wr_state(struct sAdcObject *pAdcObject){
  u16 uWriteBytes[7];
  int iStatus;

  // Need to read and store ADC32RF45X2 bootloader version before exit bootloader mode
  uWriteBytes[0] = ADC32RF45X2_BOOTLOADER_READ_OPCODE;
  uWriteBytes[1] = ((ADC32RF45X2_BOOTLOADER_VERSION_ADDRESS >> 24) & 0xFF);
  uWriteBytes[2] = ((ADC32RF45X2_BOOTLOADER_VERSION_ADDRESS >> 16) & 0xFF);
  uWriteBytes[3] = ((ADC32RF45X2_BOOTLOADER_VERSION_ADDRESS >> 8) & 0xFF);
  uWriteBytes[4] = (ADC32RF45X2_BOOTLOADER_VERSION_ADDRESS & 0xFF);
  uWriteBytes[5] = 0x00; // Reading 1 byte
  uWriteBytes[6] = 0x01; // Reading 1 byte

  iStatus = WriteI2CBytes(uADC32RF45X2MezzanineLocation + 1, ADC32RF45X2_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uWriteBytes, 7);
  if (iStatus != XST_SUCCESS){
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ADC  [%02x] Bootloader I2C write [FAILED]\r\n", uADC32RF45X2MezzanineLocation);
  }

  return ADC_STATE_INIT_BOOTLOADER_VERSION_READ_MODE;
}


static typeAdcInitState adc_init_bootloader_version_rd_state(struct sAdcObject *pAdcObject){
  u16 uReadBytes[1] = {0xffff};
  int iStatus;

  iStatus = ReadI2CBytes(uADC32RF45X2MezzanineLocation + 1, ADC32RF45X2_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uReadBytes, 1);
  if (iStatus != XST_SUCCESS){
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ADC  [%02x] Bootloader I2C read [FAILED]\r\n", uADC32RF45X2MezzanineLocation);
  }

  uADC32RF45X2BootloaderVersionMajor = (uReadBytes[0] >> 4) & 0xF;
  uADC32RF45X2BootloaderVersionMinor = uReadBytes[0] & 0xF;

  log_printf(LOG_SELECT_HARDW, LOG_LEVEL_INFO, "ADC  [%02x] Mezzanine bootloader version: %x.%x\r\n", uADC32RF45X2MezzanineLocation, uADC32RF45X2BootloaderVersionMajor, uADC32RF45X2BootloaderVersionMinor);

  return ADC_STATE_INIT_BOOTLOADER_MODE;
}


static typeAdcInitState adc_init_bootloader_state(struct sAdcObject *pAdcObject){
  u16 uWriteBytes[1];
  int iStatus;

  uWriteBytes[0] = ADC32RF45X2_LEAVE_BOOTLOADER_MODE;
  log_printf(LOG_SELECT_HARDW, LOG_LEVEL_INFO, "ADC  [%02x] Mezzanine leaving bootloader mode.\r\n", uADC32RF45X2MezzanineLocation);

  iStatus = WriteI2CBytes(uADC32RF45X2MezzanineLocation + 1, ADC32RF45X2_STM_I2C_BOOTLOADER_SLAVE_ADDRESS, uWriteBytes, 1);
  if (iStatus != XST_SUCCESS){
    log_printf(LOG_SELECT_HARDW, LOG_LEVEL_ERROR, "ADC  [%02x] Bootloader I2C write [FAILED]\r\n", uADC32RF45X2MezzanineLocation);
  }

  return ADC_STATE_INIT_STARTING_APPLICATION_MODE;
}


#define ADC_STARTING_TIMEOUT 30    /* x100 ms */

static typeAdcInitState adc_init_starting_application_state(struct sAdcObject *pAdcObject){
  /* wait state - wait for ADC/STM to be ready */
  if (pAdcObject->uWaitCount >= ADC_STARTING_TIMEOUT) {
    return ADC_STATE_INIT_APPLICATION_MODE;
  } else {
    pAdcObject->uWaitCount++;
    return ADC_STATE_INIT_STARTING_APPLICATION_MODE;
  }
}


static typeAdcInitState adc_init_application_state(struct sAdcObject *pAdcObject){
  pAdcObject->AppState = adc_app_state_table[pAdcObject->AppState](pAdcObject);

  return ADC_STATE_INIT_APPLICATION_MODE;
}


/********** Application state functions **********/

static typeAdcAppState adc_app_do_nothing(struct sAdcObject *pAdcObject){
  return ADC_STATE_APP_DO_NOTHING;
}
