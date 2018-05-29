#ifndef SENSORS_H_
#define SENSORS_H_

/*
 * sensors.h
 *
 *  Created on: 11 May 2016
 *      Author: tyronevb
 */

#include "xparameters.h"
#include "xil_types.h"
#include "constant_defs.h"
#include "register.h"
#include "i2c_master.h"
#include "flash_sdram_controller.h"
#include "icape_controller.h"
#include "isp_spi_controller.h"
#include "one_wire.h"
#include "eth_mac.h"
#include "eth_sorter.h"
#include "custom_constants.h"
#include "invalid_nack.h"

#include <stdbool.h>

// command handlers
int GetSensorDataHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);
int SetFanSpeedHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength);

// sensor related functions
void ReadFanSpeedRPM(u16 * ReadBytes, unsigned FanPage, bool OpenSwitch);
void GetAllFanSpeeds(sGetSensorDataRespT *Response);
void GetAllFanSpeedsPWM(sGetSensorDataRespT *Response);
void ReadTemperature(u16 * ReadBytes, unsigned TempSensorPage, bool OpenSwitch);
void ReadVoltageMonTemperature(u16 * ReadBytes, bool OpenSwitch);
void ReadCurrentMonTemperature(u16 * ReadBytes, bool OpenSwitch);
void GetAllTempSensors(sGetSensorDataRespT *Response);
void ReadVoltage(u16 * ReadBytes, unsigned Voltage, bool OpenSwitch);
void Read3V3Voltage(u16 * ReadBytes, bool OpenSwitch);
void GetAllVoltages(sGetSensorDataRespT *Response);
void ReadCurrent(u16 * ReadBytes, unsigned Current, bool OpenSwitch);
void GetAllCurrents(sGetSensorDataRespT *Response);
void SetFanSpeed(unsigned FanPage, float PWMPercentage, bool OpenSwitch);
void ReadMezzanineTemperature(u16 * ReadBytes, unsigned MezzaninePage, bool OpenSwitch);
void GetAllMezzanineTempSensors(sGetSensorDataRespT *Response);


// auxiliary functions
void ConfigureSwitch(unsigned SwitchSelection);

#endif /* SENSORS_H_ */
