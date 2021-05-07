#ifndef _fanctrl_h_
#define _fanctrl_h_

#ifdef __cplusplus
extern "C"{
#endif

#define RESTORE_DEFAULTS_ALL_CMD  0x12
#define FAN_CONFIG_1_2_CMD 0x3a
#define MFR_FAN_LUT_CMD 0xf2
#define STORE_DEFAULTS_ALL_CMD  0x11
#define MFR_TEMP_SENSOR_CONFIG_CMD  0xf0

int fanctrlr_configure_mux_switch();
int fanctrlr_restore_defaults();
int fanctrlr_enable_auto_fan_control();
int fanctrlr_setup_registers();
int fanctrlr_set_lut_points(u16 *setpoints);
int fanctrlr_store_defaults_to_flash();
u16 *fanctrlr_get_lut_points();
u16 fanctrlr_get_run_time(unsigned int fan_page);
u16 fanctrlr_get_pwm_avg(unsigned int fan_page);

#ifdef __cplusplus
}
#endif

#endif
