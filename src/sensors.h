#ifndef SENSORS_H_
#define SENSORS_H_

/*
 * sensors.h
 *
 *  Created on: 11 May 2016
 *      Author: tyronevb
 */

#include <stdbool.h>
#include <xil_types.h>

#include "custom_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

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
void GetAllHMCDieTemperatures(sGetSensorDataRespT *Response);


// auxiliary functions
void ConfigureSwitch(unsigned SwitchSelection);

#ifdef __cplusplus
}
#endif
#endif /* SENSORS_H_ */
