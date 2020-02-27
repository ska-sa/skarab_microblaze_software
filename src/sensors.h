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
int ReadFanSpeedRPM(u16 * ReadBytes, unsigned FanPage, bool OpenSwitch);
int ReadFanSpeedPWM(u16 * ReadBytes, unsigned FanPage, bool OpenSwitch);
void GetAllFanSpeeds(sGetSensorDataRespT *Response);
void GetAllFanSpeedsPWM(sGetSensorDataRespT *Response);
int ReadTemperature(u16 * ReadBytes, unsigned TempSensorPage, bool OpenSwitch);
int ReadVoltageMonTemperature(u16 * ReadBytes, bool OpenSwitch);
int ReadCurrentMonTemperature(u16 * ReadBytes, bool OpenSwitch);
void GetAllTempSensors(sGetSensorDataRespT *Response);
int ReadVoltage(u16 * ReadBytes, unsigned Voltage, bool OpenSwitch);
int Read3V3Voltage(u16 * ReadBytes, bool OpenSwitch);
void GetAllVoltages(sGetSensorDataRespT *Response);
int ReadCurrent(u16 * ReadBytes, unsigned Current, bool OpenSwitch);
void GetAllCurrents(sGetSensorDataRespT *Response);
void SetFanSpeed(unsigned FanPage, float PWMPercentage, bool OpenSwitch);
int ReadMezzanineTemperature(u16 * ReadBytes, unsigned MezzaninePage, bool OpenSwitch);
void GetAllMezzanineTempSensors(sGetSensorDataRespT *Response);
void GetAllHMCDieTemperatures(sGetSensorDataRespT *Response);


// auxiliary functions
int ConfigureSwitch(unsigned SwitchSelection);

#ifdef __cplusplus
}
#endif
#endif /* SENSORS_H_ */
