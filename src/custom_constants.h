/*
 * custom_constants.h
 *
 *  Created on: 07 Jun 2016
 *      Author: tyronevb
 */

#ifndef CUSTOM_CONSTANTS_H_
#define CUSTOM_CONSTANTS_H_

#include "constant_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// adding a new command for the uBlaze requires:
// * an opcode
// * new structures for the request and response
// * handlers for the new opcode
//    > template in opcode_template.c/.h

// also modify the CommandSorter function in eth_sorter.c and
// add a new condition to catch the new opcode

// custom opcodes

// increment opcode numbers by 2 (to account for response code which = request code + 1)
// NB: also change the HIGHEST_DEFINED_COMMAND constant in constant_defs.h

#define GET_SENSOR_DATA       0x0043
#define SET_FAN_SPEED       0x0045
#define BIG_READ_WISHBONE     0x0047
#define BIG_WRITE_WISHBONE      0x0049
// #define NEW_OPCODE       0x0051

// nack return opcode --> used to return when SKARAB receives an unknown opcode
#define NACK_OPCODE_RESP      0xFFFF

// other constants

// i2c constants
#define MB_I2C_BUS_ID       0x0
#define PCA9546_I2C_DEVICE_ADDRESS  0x70
#define MAX31785_I2C_DEVICE_ADDRESS 0x52
#define UCD90120A_VMON_I2C_DEVICE_ADDRESS 0x45
#define UCD90120A_CMON_I2C_DEVICE_ADDRESS 0x47

// control/monitoring - fans, temperature, voltage, current

#define FAN_CONT_SWTICH_SELECT 0x1
#define MONITOR_SWITCH_SELECT  0x02
#define PAGE_CMD         0x00
#define READ_FAN_SPEED_1_CMD   0x90
#define FAN_COMMAND_1_CMD    0x3B
#define MFR_READ_FAN_PWM_CMD   0xF3
#define VOUT_MODE_CMD      0x20
#define READ_VOUT_CMD      0x8B
#define READ_TEMPERATURE_1_CMD 0x8D

// skarab motherboard fan pages
#define LEFT_FRONT_FAN_PAGE   0
#define LEFT_MIDDLE_FAN_PAGE  1
#define LEFT_BACK_FAN_PAGE    2
#define RIGHT_BACK_FAN_PAGE   3
#define FPGA_FAN        4

// skarab motherboard temperature sensor pages
#define INLET_TEMP_SENSOR_PAGE    13
#define OUTLET_TEMP_SENSOR_PAGE   14
#define FPGA_TEMP_DIODE_ADC_PAGE  10
#define MEZZANINE_0_TEMP_ADC_PAGE 17
#define MEZZANINE_1_TEMP_ADC_PAGE 18
#define MEZZANINE_2_TEMP_ADC_PAGE 19
#define MEZZANINE_3_TEMP_ADC_PAGE 20
#define FAN_CONT_TEMP_SENSOR_PAGE 12

#define VOLTAGE_MON_TEMP_SENSOR_PAGE 30 // custom page created for handling voltage/current mon temps
#define CURRENT_MON_TEMP_SENSOR_PAGE 31

// UCD90120A voltage monitor pages
#define P12V2_VOLTAGE_MON_PAGE        0
#define P12V_VOLTAGE_MON_PAGE       1
#define P5V_VOLTAGE_MON_PAGE        2
#define P3V3_VOLTAGE_MON_PAGE       3
#define P2V5_VOLTAGE_MON_PAGE       4
#define P1V8_VOLTAGE_MON_PAGE       5
#define P1V2_VOLTAGE_MON_PAGE       6
#define P1V0_VOLTAGE_MON_PAGE       7
#define P1V8_MGTVCCAUX_VOLTAGE_MON_PAGE   8
#define P1V0_MGTAVCC_VOLTAGE_MON_PAGE   9
#define P1V2_MGTAVTT_VOLTAGE_MON_PAGE   10
#define P3V3_CONFIG_VOLTAGE_MON_PAGE    11  // rev 1 mb (deprecated on rev2)
#define P5VAUX_VOLTAGE_MON_PAGE       11
#define PLUS3V3CONFIG02_ADC_PAGE      22

// UCD90120A current monitor pages
#define P12V2_CURRENT_MON_PAGE        0
#define P12V_CURRENT_MON_PAGE       1
#define P5V_CURRENT_MON_PAGE        2
#define P3V3_CURRENT_MON_PAGE       3
#define P2V5_CURRENT_MON_PAGE       4
#define P3V3_CONFIG_CURRENT_MON_PAGE    5
#define P1V2_CURRENT_MON_PAGE       6
#define P1V0_CURRENT_MON_PAGE       7
#define P1V8_MGTVCCAUX_CURRENT_MON_PAGE   8
#define P1V0_MGTAVCC_CURRENT_MON_PAGE   9
#define P1V2_MGTAVTT_CURRENT_MON_PAGE   10
#define P1V8_CURRENT_MON_PAGE       11

// HMC Mezzanine Sites - used for i2c reads
#define HMC_Mezzanine_Site_1 1
#define HMC_Mezzanine_Site_2 2
#define HMC_Mezzanine_Site_3 3
#define HMC_I2C_Address 0x10
#define HMC_Temperature_Write_Reg 0x2b0004
#define HMC_Temperature_Write_Command 0x8000000a
#define HMC_Die_Temperature_Reg 0x2b0000


// custom structures
// structs for the request and response packets of commands

// request packet
typedef struct sNewOpCodeReq{
  sCommandHeaderT Header;

  // other members here

} sNewOpCodeReqT;

// response packet
typedef struct sNewOpCodeRespT{
  sCommandHeaderT Header;

  // other members here

  // padding - number of padding words
  // must be calculated
  //
  u16       uPadding[2];
} sNewOpCodeRespT;

// request and response packet structures

// GetSensorData
typedef struct sGetSensorDataReq {
  sCommandHeaderT Header;
} sGetSensorDataReqT;

typedef struct sGetSensorDataResp {
	sCommandHeaderT Header;
	u16				uSensorData[106];
	u16				uPadding[3];
} sGetSensorDataRespT;

// SetFanSpeed
typedef struct sSetFanSpeedReq{
  sCommandHeaderT Header;
  u16       uFanPage;
  u16       uPWMSetting;
} sSetFanSpeedReqT;

typedef struct sSetFanSpeedResp{
  sCommandHeaderT Header;
  u16       uNewFanSpeedPWM;
  u16       uNewFanSpeedRPM;
  u16       uPadding[7];
} sSetFanSpeedRespT;

typedef struct sBigWriteWishboneReq {
  sCommandHeaderT Header;
    u16       uStartAddressHigh;
    u16       uStartAddressLow;
    u16       uWriteData[994];
    u16       uNumberOfWrites;
} sBigWriteWishboneReqT;

typedef struct sBigWriteWishboneResp {
  sCommandHeaderT Header;
    u16       uStartAddressHigh;
    u16       uStartAddressLow;
    u16       uNumberOfWritesDone;
    u16       uPadding[6];
} sBigWriteWishboneRespT;

typedef struct sBigReadWishboneReq {
  sCommandHeaderT Header;
    u16       uStartAddressHigh;
    u16       uStartAddressLow;
    u16       uNumberOfReads;
} sBigReadWishboneReqT;

typedef struct sBigReadWishboneResp {
  sCommandHeaderT Header;
    u16       uStartAddressHigh;
    u16       uStartAddressLow;
    u16       uNumberOfReads;
    u16       uReadData[994];
} sBigReadWishboneRespT;

// packet structure for NACK response
typedef struct sInvalidOpcodeResp {
  sCommandHeaderT Header;
  u16       uPadding[9];
} sInvalidOpcodeRespT;

#ifdef __cplusplus
}
#endif
#endif /* CUSTOM_CONSTANTS_H_ */
