#include <xil_types.h>
#include <xstatus.h>
#include <xil_printf.h>
#include <stdlib.h>

#include "fault_log.h"
#include "i2c_master.h"
#include "constant_defs.h"
#include "custom_constants.h"
#include "logging.h"

#define NUM_SECONDS_IN_A_DAY 86400u

/* structure representing the time values of a UCD90120A log entry */
struct log_time_entry{
  u32 uDays;
  u32 uMilliSeconds;
};

/* forward declarations */
static int convert_log_time_to_seconds(const struct log_time_entry *const te, u32 *seconds);
static int get_fanctrl_run_time_count_seconds(u32 *s);
static int get_monitor_run_time_count_seconds(const u16 slave_addr, u32 *s);
static int get_time_of_latest_monitor_log_entry_seconds(const u16 slave_addr, u32 *seconds);

/*
 * wraps up the set of i2c commands to retrieve a single log entry from the
 * TI-UCD90120A monitoring device for the given index. The device is set up
 * to treat the logging data flash as a fifo of 16 entries max.
 */

#define LOGGED_FAULT_DETAIL_INDEX 0xeb
#define LOGGED_FAULT_DETAIL       0xec
#define MFR_TIME_COUNT            0xdd
#define RUN_TIME_CLOCK            0xd7

/* ensure that data_out points to a buffer with a size of 10 bytes */
static int get_log_entry_UCD90120A(const u16 slave_addr, const u8 index, u8 *data_out){
  int status, i;
  u16 write_buff[3];
#define LOG_ENTRY_LEN 11  /* BYTE_COUNT + payload */
  u16 read_buff[LOG_ENTRY_LEN];
  const u16 sw = MONITOR_SWITCH_SELECT;
  /* static u8 output_data[LOG_ENTRY_LEN - 1]; */

  if (NULL == data_out){
    return XST_FAILURE;
  }

  /* sanity check */
  if ((index > NUM_LOG_ENTRIES) || (index < 0)){
    return XST_FAILURE;
  }

  /* all TI monitoring chips are on PCA9546 switch channel 2 */
  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  write_buff[0] = LOGGED_FAULT_DETAIL_INDEX;
  write_buff[1] = (u16) index;
  write_buff[2] = 0;    /* don't care */

  /* set the LOGGED_FAULT_DETAIL_INDEX to the given index {0 to 15} */
  status = WriteI2CBytes(MB_I2C_BUS_ID, slave_addr, write_buff, 3);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /*TODO: should we have a slight delay here to ensure device is ready after i2c write?*/

  /* now read the log entry */
  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, slave_addr, LOGGED_FAULT_DETAIL, read_buff, LOG_ENTRY_LEN);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  for (i = 0; i < (LOG_ENTRY_LEN - 1); i++){
    data_out[i] = (u8) read_buff[i + 1];     /* skip the BYTE_COUNT at the head of bytes read */
  }

  return XST_SUCCESS;
}



/* NOTE: returns the number of entries and not the index. */
static int get_num_log_entries_UCD90120A(const u16 slave_addr, u8 *num_entries){
  const u16 sw = MONITOR_SWITCH_SELECT;
  int status;
#define LOG_INDEX_LEN 2
  u16 read_buff[LOG_INDEX_LEN];

  if (NULL == num_entries){
    return XST_FAILURE;
  }

  /* all TI monitoring chips are on PCA9546 switch channel 2 */
  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* now read the log count */
  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, slave_addr, LOGGED_FAULT_DETAIL_INDEX, read_buff, LOG_INDEX_LEN);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  *num_entries = (u8) read_buff[1];

  /* sanity check */
  if ((*num_entries) > (NUM_LOG_ENTRIES)){
    return XST_FAILURE;
  }

  return XST_SUCCESS;
}

static int get_log_entry_decoded(const u16 slave_addr, const u8 index, sLogDataEntryT *log_out){
  u8 entry[10];
  int status;
  u16 write_buff[2];
  u16 read_buff;
  const u16 sw = MONITOR_SWITCH_SELECT;
  struct log_time_entry time_entry;
  u32 when = 0; /* fault time seconds count */
  u32 now = 0;  /* current run-time seconds count */
  u32 delta = 0;

  if (NULL == log_out){
    return XST_FAILURE;
  }

  status = get_log_entry_UCD90120A(slave_addr, index, entry);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  for (int i=0;i<10;i++){
    xil_printf("%d ", entry[i]);
  }
  xil_printf("\r\n");

  /* Both the current and voltage monitor devices are actually reading voltages.
   * In the case of the current monitor, this voltage is simple converted using
   * the sense resistor value and op-amp gain by the host. Therefore, both needs
   * to return the voltage scaling factor in the VOUT_MODE register. */

  /* decode the data entry */
  /* TODO: reference the spec data sheet here */
  log_out->uPageSpecific = (entry[4] >> 7) & 0x1u;
  log_out->uFaultType = (entry[4] >> 3) & 0xfu;
  log_out->uPage = (((entry[4] & 0x7u) << 1) | ((entry[5] >> 7) & 0x1u)) & 0xf;
  xil_printf("index %d page %d\r\n", index, log_out->uPage);
  log_out->uFaultValue = ((entry[9] << 8) & 0xff00) | (entry[8] & 0xff);

  /* the days field is only 23-bits wide which we are fitting into a 32-bit word */
  time_entry.uDays = ((entry[5] << 16) & 0x7f0000) | ((entry[6] << 8) & 0xff00) | (entry[7] & 0xff);
  time_entry.uMilliSeconds = ((entry[0] << 24) & 0xff000000) |
                             ((entry[1] << 16) & 0x00ff0000) |
                             ((entry[2] <<  8) & 0x0000ff00) |
                             ( entry[3]        & 0x000000ff);

  status = convert_log_time_to_seconds(&time_entry, &when);
  if (XST_SUCCESS != status){
    /* this would probably never fail - but just for completeness sake, populate with zero */
    when = 0;
  }

  status = get_monitor_run_time_count_seconds(slave_addr, &now);
  if (XST_SUCCESS != status){
    delta = 0xffffffff;    /* TODO what should this be? Perhaps an unrealistic value? */
  } else {
    delta = now - when; /* now_in_seconds - log_fault_time_in_seconds */
  }

  xil_printf("index %d now %d when %d\r\n", index, now, when);

  log_out->uSeconds[0] = (delta >> 16) & 0xffff;        /* most-significant word16 first */
  log_out->uSeconds[1] = (delta & 0xffff);

#if 0
  log_out->uMilliSeconds[0] = ((entry[0] << 8) & 0xff00) | (entry[1] & 0xff); /* most-significant word16 first */
  log_out->uMilliSeconds[1] = ((entry[2] << 8) & 0xff00) | (entry[3] & 0xff);

  log_out->uDays[0] = entry[5] & 0x7f;
  log_out->uDays[1] = ((entry[6] << 8) & 0xff00) | (entry[7] & 0xff);
#endif

  /* get VOUT_MODE idepending on page read above */
  /* TODO: code duplication alert since this functionality is in sensors.c also */

  /* ensure that the i2c switch correctly configured - may be unnecessary but included for
   * completeness sake in case get_vout_mode  gets encapsulated into its own function. */
  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* set relevant page */
  write_buff[0] = PAGE_CMD;
  write_buff[1] = log_out->uPage;
  status = WriteI2CBytes(MB_I2C_BUS_ID, slave_addr, write_buff, 2);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* now read VOUT_MODE */
  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, slave_addr, VOUT_MODE_CMD, &read_buff, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  log_out->uValueScale = read_buff; /*FIXME*/

  return XST_SUCCESS;
}


/* fault time related funtions */

static int convert_log_time_to_seconds(const struct log_time_entry *const te, u32 *seconds){
  /* TODO: overflow of seconds not taken into account since this should not happen in our application */
  unsigned long int s;

  if (!seconds){
    return XST_FAILURE;
  }

  if (!te){
    *seconds = 0;
    return XST_FAILURE;
  }

  s = (te->uDays * NUM_SECONDS_IN_A_DAY) + (te->uMilliSeconds / 1000u);

  *seconds = (u32) s;
  /* TODO: could return an overflow too? */

  return XST_SUCCESS;
}


static int get_monitor_run_time_count_seconds(const u16 slave_addr, u32 *s){
  const u16 sw = MONITOR_SWITCH_SELECT;
  int status;
#define MONITOR_TIME_COUNT_RD_LEN  9
  u16 read_buff[MONITOR_TIME_COUNT_RD_LEN];
  struct log_time_entry time;

  if (!s){
    return XST_FAILURE;
  }

  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    *s = 0; /* TODO: should this be an unrealistic value? */
    return XST_FAILURE;
  }

  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, slave_addr, RUN_TIME_CLOCK, read_buff, MONITOR_TIME_COUNT_RD_LEN);
  if (XST_SUCCESS != status){
    *s = 0;
    return XST_FAILURE;
  }

  /* first byte of payload is the byte count */
  time.uMilliSeconds = ((read_buff[1] << 24) & 0xff000000) |
                       ((read_buff[2] << 16) & 0x00ff0000) |
                       ((read_buff[3] <<  8) & 0x0000ff00) |
                       ( read_buff[4]        & 0x000000ff);

  time.uDays = ((read_buff[5] << 24) & 0xff000000) |
               ((read_buff[6] << 16) & 0x00ff0000) |
               ((read_buff[7] <<  8) & 0x0000ff00) |
               ( read_buff[8]        & 0x000000ff);

  /* convert to seconds */
  return convert_log_time_to_seconds(&time, s);
}

/* TODO: perhaps split out vmon and cmon functionality */
static int set_all_monitor_run_time_count_seconds(const u32 seconds){
  int status;
  const u16 sw = MONITOR_SWITCH_SELECT;
  u32 days;
  u32 milliseconds;
#define MONITOR_TIME_COUNT_WR_LEN  10   /* reg_addr[1] + byte_count[1] + payload[8] */
  u16 write_buff[MONITOR_TIME_COUNT_WR_LEN];

  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  days = seconds / NUM_SECONDS_IN_A_DAY;    /* quotient */
  milliseconds = (seconds % NUM_SECONDS_IN_A_DAY) * 1000; /* remainder x 1000 */

  write_buff[0] = RUN_TIME_CLOCK;
  write_buff[1] = 8;  /* byte count */

  write_buff[2] = (milliseconds >> 24) & 0xff;
  write_buff[3] = (milliseconds >> 16) & 0xff;
  write_buff[4] = (milliseconds >>  8) & 0xff;
  write_buff[5] = (milliseconds      ) & 0xff;

  write_buff[6] = (days >> 24) & 0xff;
  write_buff[7] = (days >> 16) & 0xff;
  write_buff[8] = (days >>  8) & 0xff;
  write_buff[9] = (days      ) & 0xff;

  /* set current monitor counter */
  status = WriteI2CBytes(MB_I2C_BUS_ID, UCD90120A_CMON_I2C_DEVICE_ADDRESS, write_buff, MONITOR_TIME_COUNT_WR_LEN);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* set voltage monitor counter */
  status = WriteI2CBytes(MB_I2C_BUS_ID, UCD90120A_VMON_I2C_DEVICE_ADDRESS, write_buff, MONITOR_TIME_COUNT_WR_LEN);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  return XST_SUCCESS;
}


static int get_fanctrl_run_time_count_seconds(u32 *s){
  /*
   * the fan controller MFR_TIME_COUNT value is the authoritative counter for
   * time-stamping the log entries.
   * NOTE: this is a 32-bit register and ovrflows are not taken into account since
   * it will probably not be an issue in the lifetime of the device and this
   * application (?)
   */
  const u16 sw = FAN_CONT_SWTICH_SELECT;
  int status;
#define FANCTRL_TIME_COUNT_LEN  4
  u16 read_buff[FANCTRL_TIME_COUNT_LEN];

  if (!s){
    return XST_FAILURE;
  }

  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    *s = 0; /* TODO: should this be an unrealistic value? */
    return XST_FAILURE;
  }

  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_TIME_COUNT, read_buff, FANCTRL_TIME_COUNT_LEN);
  if (XST_SUCCESS != status){
    *s = 0;
    return XST_FAILURE;
  }

  *s = ((read_buff[3] << 24) & 0xff000000) |
       ((read_buff[2] << 16) & 0x00ff0000) |
       ((read_buff[1] <<  8) & 0x0000ff00) |
       ( read_buff[0]        & 0x000000ff);

  return XST_SUCCESS;
}

/* get the time of the last log entry of a specified monitoring device */
static int get_time_of_latest_monitor_log_entry_seconds(const u16 slave_addr, u32 *seconds){
  int status;
  const u16 sw = MONITOR_SWITCH_SELECT;
  u8 num = 0, index;
  u8 entry[10];
  struct log_time_entry time_entry;

  if (!seconds){
    return XST_FAILURE;
  }

  /* TODO: could do a sanity check on the slave addr too, but this will be handled by
   * the i2c bus anyway - just ensure that we fail gracefully */

  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    *seconds = 0; /* TODO: should this be an unrealistic value? */
    return XST_FAILURE;
  }

  status = get_num_log_entries_UCD90120A(slave_addr, &num);
  if (XST_SUCCESS != status){
    *seconds = 0; /* TODO: should this be an unrealistic value? */
    return XST_FAILURE;
  }

  if (0 == num){
    /* no entries in the log - let's fail here and let the calling function handle it */
    *seconds = 0;
    return XST_FAILURE;
  }

  /* set the index to the latest entry */
  index = num - 1;

  status = get_log_entry_UCD90120A(slave_addr, index, entry);
  if (XST_SUCCESS != status){
    *seconds = 0; /* TODO: should this be an unrealistic value? */
    return XST_FAILURE;
  }

  /* the days field is only 23-bits wide which we are fitting into a 32-bit word */
  time_entry.uDays = ((entry[5] << 16) & 0x7f0000) | ((entry[6] << 8) & 0xff00) | (entry[7] & 0xff);
  time_entry.uMilliSeconds = ((entry[0] << 24) & 0xff000000) |
                             ((entry[1] << 16) & 0x00ff0000) |
                             ((entry[2] <<  8) & 0x0000ff00) |
                             ( entry[3]        & 0x000000ff);

  return convert_log_time_to_seconds(&time_entry, seconds);
}


/* Wraps up the logic to sync the fan controller, current monitor and voltage monitor
 * run time counters. This is used to time stamp the fault logs. This function will
 * be called at start-up and then again periodically after that (perhaps every
 * 15mins) to re-sync the devices since their clocks are not accurate with respect to
 * each other. */
/*
 * Sync the TI UCD90120A current and voltage monitoring devices' time count with that
 * of the MAX31785 fan controller. This is important for establishing a reference
 * point for the monitoring fault logs when retrieved from the devices since their
 * native counter value is stored in volatile memory and hence gives no information
 * about a log timestamp after a power-cycle (which is the main mechanism used on
 * site to reboot the skarabs). The time counters will be sync'd at startup.
 */

/*
 * DEV NOTES:
 * 1) register overflows will not be handled since this will only be a problem in
 * 100+ years time...
 *
 * 2) vmon and cmon devices seem to run at different speeds to the fan controller
 * timer.  They do have a run time clock trim register with which one can speed up or
 * slow down the clock to correct this but this may not be feasible since this value
 * may differ across all the 280+ skarabs. Perhaps a more practical solution would be
 * to re-sync the counters every n-minutes (perhaps every 15minutes) since through
 * experimentation it is seen that the v-/c-mon counters drift by about 1% over time.
 * The clock just needs to be accurate enough in order to timestamp the log data and
 * thereby enabling us to determine when a fault occured.
 */
int log_time_sync_devices(void){
  u32 fanctrl_time;
  u32 latest_cmon_log_time;
  u32 latest_vmon_log_time;
  u32 time = 0;
  int status;

  /* get fan controller counter. This counter is automatically set by the device to a
   * value which is saved every hour into flash.*/
  status = get_fanctrl_run_time_count_seconds(&fanctrl_time);
  if (XST_SUCCESS != status){
    /* critical */
    return XST_FAILURE;
  }

  /* get time of the latest current fault log */
  status = get_time_of_latest_monitor_log_entry_seconds(UCD90120A_CMON_I2C_DEVICE_ADDRESS, &latest_cmon_log_time);
  if (XST_SUCCESS != status){
    /* TODO */
    latest_cmon_log_time = 0;
  }

  /* get time of the latest voltage fault log */
  status = get_time_of_latest_monitor_log_entry_seconds(UCD90120A_VMON_I2C_DEVICE_ADDRESS, &latest_vmon_log_time);
  if (XST_SUCCESS != status){
    /* TODO */
    latest_vmon_log_time = 0;
  }

  /* TODO: take the fanctrlr latest log into account as well... */

  /* choose the largest time */
  time = fanctrl_time >= latest_cmon_log_time ? fanctrl_time : latest_cmon_log_time;
  time = time >= latest_vmon_log_time ? time : latest_vmon_log_time;
  /* TODO: we know that the latest fault log time should never be ahead of the fanctrl time by more than 60mins
   * (59m59s). This should only happen when the fanctrl time count overflows (in 130+ years). */

  /* NOTE: not possible to set the fanctrlr time count...can only be cleared */

  /* now sync the devices - may be out by a few milliseconds due to i2c op latencies */
  return set_all_monitor_run_time_count_seconds(time);
}


/* fan controller logs (temperature) */

/* ensure data_out > 255 bytes */
static int read_next_fault_log_page_MAX31785_raw(u16 *const data_out){
  const u16 sw = FAN_CONT_SWTICH_SELECT;
  int status = XST_FAILURE;
  int i;

  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

#define MFR_NV_FAULT_LOG 0xdc
#define FAULT_PAGE_LEN_MAX34785 255
  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_NV_FAULT_LOG, data_out, FAULT_PAGE_LEN_MAX34785);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* print the raw data */
  for (i = 0; i < 255; i++){
    log_printf(LOG_SELECT_CTRL, LOG_LEVEL_TRACE, "%4x%s", data_out[i], ((i % 16 == 0) && (i != 0)) ? "\r\n" : " ");
    //xil_printf("%4x", buffer[i]);
    //xil_printf("%s", i % 16 == 0 ? "\r\n" : " ");
  }
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_TRACE, "\r\n");

  return XST_SUCCESS;
}



static int get_fanctrlr_log_entry_decoded(sFanCtrlrLogDataEntryT *log_out){
  u16 *buffer = NULL;
  int status = XST_FAILURE;

#if 0
  if (!de){
    /* should actually assert here */
    return XST_FAILURE;
  }
#endif

  /* TODO: should probably move this higher up since this will cause
   * allocation/dealloction on each call. Unnecessary overhead. */
  buffer = (u16 *) malloc(FAULT_PAGE_LEN_MAX34785 * sizeof(u16));
  if (NULL == buffer){
    goto failure;
  }

  status = read_next_fault_log_page_MAX31785_raw(buffer);
  if (XST_SUCCESS != status){
    goto failure;
  }

  /* parse the data */
  /* STATUS_WORD logged at byte offset 10 (LSB) and 11 (MSB) */

  log_out->uFaultLogIndex = buffer[1] & 0x00ff;   /*  index always between 0 and 14 */

  log_out->uFaultLogCount = ((buffer[3] << 8) & 0xff00) | ((buffer[2] & 0x00ff));
  log_out->uStatusWord = ((buffer[11] << 8) & 0xff00) | ((buffer[10] & 0x00ff));
  log_out->uStatusVout_17_18 = ((buffer[13] << 8) & 0xff00) | ((buffer[12] & 0x00ff));
  log_out->uStatusVout_19_20 = ((buffer[15] << 8) & 0xff00) | ((buffer[14] & 0x00ff));
  log_out->uStatusVout_21_22 = ((buffer[17] << 8) & 0xff00) | ((buffer[16] & 0x00ff));
  log_out->uStatusMfr_6_7 = ((buffer[19] << 8) & 0xff00) | ((buffer[18] & 0x00ff));
  log_out->uStatusMfr_8_9 = ((buffer[21] << 8) & 0xff00) | ((buffer[20] & 0x00ff));
  log_out->uStatusMfr_10_11 = ((buffer[23] << 8) & 0xff00) | ((buffer[22] & 0x00ff));
  log_out->uStatusMfr_12_13 = ((buffer[25] << 8) & 0xff00) | ((buffer[24] & 0x00ff));
  log_out->uStatusMfr_14_15 = ((buffer[27] << 8) & 0xff00) | ((buffer[26] & 0x00ff));
  log_out->uStatusMfr_16_none = ((buffer[29] << 8) & 0xff00) | ((buffer[28] & 0x00ff));
  log_out->uStatusFans_0_1 = ((buffer[31] << 8) & 0xff00) | ((buffer[30] & 0x00ff));
  log_out->uStatusFans_2_3 = ((buffer[33] << 8) & 0xff00) | ((buffer[32] & 0x00ff));
  log_out->uStatusFans_4_5 = ((buffer[35] << 8) & 0xff00) | ((buffer[34] & 0x00ff));

  status = XST_SUCCESS;

failure:
  free(buffer);
  return status;
}

/* interface functions */

#define MFR_MODE  0xd1
#define MFR_MODE_LEN 2
int clear_fanctrlr_logs(void){
  const u16 sw = FAN_CONT_SWTICH_SELECT;
  int status;
  u16 readbytes[MFR_MODE_LEN];
  u16 writebytes[MFR_MODE_LEN + 1];

  /* set the i2c switch to select the appropriate bus for the fan controller */
  status = WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, (u16 *) &sw, 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* set bit_14 of MFR_MODE (cmd 0xd1) register in fan controller */

  /* read value*/
  status = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_MODE, readbytes, MFR_MODE_LEN);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  /* set bit 14 */
  writebytes[0] = MFR_MODE;
  writebytes[1] = readbytes[0];
  writebytes[2] = readbytes[1] | 0x40;

  /* write back new value */
  status = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, writebytes, MFR_MODE_LEN + 1);
  if (XST_SUCCESS != status){
    return XST_FAILURE;
  }

  return XST_SUCCESS;
}


int get_num_current_log_entries(u8 *num_entries){
  return get_num_log_entries_UCD90120A(UCD90120A_CMON_I2C_DEVICE_ADDRESS, num_entries);
}

int get_current_log_entry(const u8 index, sLogDataEntryT *log_out){
  return get_log_entry_decoded(UCD90120A_CMON_I2C_DEVICE_ADDRESS, index, log_out);
}

int get_num_voltage_log_entries(u8 *num_entries){
  return get_num_log_entries_UCD90120A(UCD90120A_VMON_I2C_DEVICE_ADDRESS, num_entries);
}

int get_voltage_log_entry(const u8 index, sLogDataEntryT *log_out){
  return get_log_entry_decoded(UCD90120A_VMON_I2C_DEVICE_ADDRESS, index, log_out);
}

int get_fanctrlr_log_entry_next(sFanCtrlrLogDataEntryT *log_out){
  return get_fanctrlr_log_entry_decoded(log_out);
}


