/* rvw - SARAO - 2020
   MAX31785 fpga fan controller helper functions
 */

#include <xil_types.h>
#include <xstatus.h>

#include "fanctrl.h"
#include "logging.h"
#include "custom_constants.h"
#include "constant_defs.h"
#include "i2c_master.h"
#include "sensors.h"
#include "delay.h"

/* #define DRYRUN */
#ifdef DRYRUN
static int write_i2c_bytes_dryrun(u16 uId, u16 uSlaveAddress, u16 * uWriteBytes, u16 uNumBytes);
#endif

int fanctrlr_configure_mux_switch(){
  return ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
}


int fanctrlr_restore_defaults(){
  u16 wr_buffer[1] = {0};
  int ret;

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] restoring fan controller defaults...");
  wr_buffer[0] = RESTORE_DEFAULTS_ALL_CMD;
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 1);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 1);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  return XST_SUCCESS;
}



int fanctrlr_enable_auto_fan_control(){
  u16 wr_buffer[3] = {0};
  int ret;

  /*set the page for the fpga fan */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA FAN page %d...", FPGA_FAN);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_FAN;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] enabling automatic fan control; setting FAN_COMMAND_1 reg to 0xffff...");
  wr_buffer[0] = FAN_COMMAND_1_CMD; // command code for I2C
  wr_buffer[1] = 0xff;
  wr_buffer[2] = 0xff;
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  return XST_SUCCESS;
}



/* this function prepares the required register for automaitc fan control */
int fanctrlr_setup_registers(){
  int ret;
  u16 wr_buffer[3] = {0};
  u16 rd_buffer[2] = {0};

  /*set the page for the fpga fan */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA FAN page %d...", FPGA_FAN);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_FAN;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] ramp down the fpga fan to 0%% pwm...");
  wr_buffer[0] = FAN_COMMAND_1_CMD;
  wr_buffer[1] = 0x0;   /* NOTE: this is the LSB of fan pwm percentage in hex */
  wr_buffer[2] = 0x0;   /* NOTE: this is the MSB of fan pwm percentage in hex */
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA_TEMP_DIODE page %d...", FPGA_TEMP_DIODE_ADC_PAGE);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_TEMP_DIODE_ADC_PAGE;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] setting bit4 of MFR_TEMP_SENSOR_CONFIG register...");
  ret = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_TEMP_SENSOR_CONFIG_CMD, rd_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  /* NOTE: delay of 5ms already implemented in pbmus read function for max31785 ic */
  wr_buffer[0] = MFR_TEMP_SENSOR_CONFIG_CMD; // command code for I2C
  wr_buffer[1] = rd_buffer[0] | 0x10u; /* set bit 4 of lsb */
  wr_buffer[2] = rd_buffer[1];
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 3);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] MFR_TEMP_SENSOR_CONFIG: rd=>x%x x%x; set bit4; wr=>x%x x%x\r\n", rd_buffer[1], rd_buffer[0], wr_buffer[2], wr_buffer[1]);


  /*set the page for the fpga fan */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA FAN page %d...", FPGA_FAN);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_FAN;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] clearing bit6 of FAN_CONFIG_1_2 register...");
  ret = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, FAN_CONFIG_1_2_CMD, rd_buffer, 1);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  /* NOTE: delay of 5ms already implemented in pbmus read function for max31785 ic */
  wr_buffer[0] = FAN_CONFIG_1_2_CMD; // command code for I2C
  wr_buffer[1] = rd_buffer[0] & 0xbfu; /* clear bit6 */
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] FAN_CONFIG_1_2: rd=>x%x; clear bit6; wr=>x%x\r\n", rd_buffer[0], wr_buffer[1]);

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] MAX31785 fan controller registers ready\r\n");

  return XST_SUCCESS;
}

#if 0
  format: temp0-lsb,temp0-msb,pwm0-lsb,pwm0-msb,temp1-lsb,temp1-msb,pwm1-lsb,pwm1-msb,...,... till n=31
#endif

#define NUM_OF_LUT_DATA_POINTS (32)
#define SIZE_OF_WR_BUFF_BYTES  (NUM_OF_LUT_DATA_POINTS + 1)    /* '+1' is space for the cmd id */
int fanctrlr_set_lut_points(u16 *setpoints){
  u16 wr_buffer[SIZE_OF_WR_BUFF_BYTES] = {0};
  int ret;
  int i;

  /* test for increasing monotonicity */
  /* pwm values at indices 0,2,4,6... => check pwm0 < pwm1 < pwm2 < ... < pwm7 */
  for (i = 2; i < 16; i+=2){
    if (setpoints[i] < setpoints[i-2]){
      log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] pwm setpoint %d fails monotonicity\r\n", i);
      return XST_FAILURE;
    }
  }

  /* temperature values at indices 1,3,5,7... => check temp0 < temp1 < temp2 < ... < temp7 */
  for (i = 3; i < 16; i+=2){
    if (setpoints[i] < setpoints[i-2]){
      log_printf(LOG_SELECT_CTRL, LOG_LEVEL_ERROR, "CTRL [..] temperature setpoint %d fails monotonicity\r\n", i);
      return XST_FAILURE;
    }
  }

  /*set the page for the fpga fan */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA FAN page %d...", FPGA_FAN);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_FAN;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  wr_buffer[0] = MFR_FAN_LUT_CMD;

  /* data-marshalling of the set point data into the write buffer */
  for (i = 0; i < 16; i++){
    /* TODO: perhaps do some sanity checks on the data - upper/lower bounds, increasing steps, etc. */
    wr_buffer[i*2+1]   = (u8) (setpoints[i] & 0xff);
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "0x%x ", wr_buffer[i*2+1]);
    wr_buffer[i*2+2] = (u8) ((setpoints[i] >> 8 )& 0xff);
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "0x%x\r\n", wr_buffer[i*2+2]);
  }

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] writing setpoints to lookup table...");

#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, SIZE_OF_WR_BUFF_BYTES);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, SIZE_OF_WR_BUFF_BYTES);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  return XST_SUCCESS;
}


int fanctrlr_store_defaults_to_flash(){
  u16 wr_buffer[1] = {0};
  int ret;

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] storing fan controller defaults to flash...");
  wr_buffer[0] = STORE_DEFAULTS_ALL_CMD;
#ifdef DRYRUN
  ret = write_i2c_bytes_dryrun(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 1);
#else
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 1);
#endif
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return XST_FAILURE;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  return XST_SUCCESS;
}

#define NUM_OF_SETPOINTS 16
u16 *fanctrlr_get_lut_points(){
  static u16 lut_buffer[NUM_OF_SETPOINTS];
  u16 rd_buffer[NUM_OF_LUT_DATA_POINTS];
  u16 wr_buffer[3] = {0};
  int ret;
  int i;

  /*set the page for the fpga fan */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] selecting FPGA FAN page %d...", FPGA_FAN);
  wr_buffer[0] = PAGE_CMD; // command code for I2C
  wr_buffer[1] = FPGA_FAN;
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, wr_buffer, 2);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return NULL;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");


  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "CTRL [..] reading fpga fan control lookup table...");
  ret = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_FAN_LUT_CMD, rd_buffer, NUM_OF_LUT_DATA_POINTS);
  if (XST_FAILURE == ret){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[failed]\r\n");
    return NULL;
  }
  Delay(500000);    /*  sleep for 0.5s */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "[ok]\r\n");

  /* pack the data into the lut_buffer */
  for (i = 0; i < NUM_OF_SETPOINTS; i++){
    lut_buffer[i] = (rd_buffer[i*2] & 0xff) | ((rd_buffer[i*2+1] & 0xff) << 8);
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_DEBUG, "0x%x\r\n", lut_buffer[i]);
  }

  /* display a table of setpoints */
  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "setpoint: (temp,pwm)\r\n");
  for (i = 0; i < 8; i++){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO, "%d: (%d,%d)\r\n", i, lut_buffer[i*2], lut_buffer[i*2+1]);
  }

  return lut_buffer;
}

#ifdef DRYRUN
/* this is a function used for testing - it merely writes the i2c bytes to the console */
static int write_i2c_bytes_dryrun(u16 uId, u16 uSlaveAddress, u16 * uWriteBytes, u16 uNumBytes){
  u16 i;

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO,"i2c wr: x%x x%x ", uId, uSlaveAddress);

  for (i = 0; i < uNumBytes; i++){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO,"x%x ", uWriteBytes[i]);
  }

  log_printf(LOG_SELECT_CTRL, LOG_LEVEL_INFO,"\r\n");

  return XST_SUCCESS;
}
#endif
