/*
 * sensors.c
 *
 *  Created on: 11 May 2016
 *      Author: tyronevb
 *
 * ------------------------------------------------------------------------------
 *  DESCRIPTION :
 *
 *  This file contains the implementation of sensor reading functions.
 * ------------------------------------------------------------------------------*/

#include <xstatus.h>

#include "sensors.h"
#include "i2c_master.h"
#include "custom_constants.h"
#include "mezz.h"

// function definitions

//=================================================================================
//  GetSensorDataHandler
//--------------------------------------------------------------------------------
//  This method executes the GetSensorDataHandler. This handler polls all the various
//  sensors on the SKARAB motherboard and returns all the sensor data to the host.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int GetSensorDataHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

  sGetSensorDataReqT *Command = (sGetSensorDataReqT *) pCommand;
  sGetSensorDataRespT *Response = (sGetSensorDataRespT *) uResponsePacketPtr;
  u8 i;

  if (uCommandLength < sizeof(sGetSensorDataReqT)){
    return XST_FAILURE;
  }

  // Create response packet header
  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  /* clear all status bits - will be set upon error */
  for (i = 0; i < 2; i++){
    Response->uStatusBits[i] = 0;
  }

	// sensor reads
	// automatically populates the fields for the reponse packet
	GetAllFanSpeeds(Response); // fan speed rpm
	GetAllFanSpeedsPWM(Response); // fan speed pwm
	GetAllTempSensors(Response); // temperature sensors (excl. mezzanine sites)
	GetAllVoltages(Response); // psu voltages
	GetAllCurrents(Response); // psu currents
	GetAllMezzanineTempSensors(Response); // mezzanine temperatures
  GetAllHMCDieTemperatures(Response); // HMC Die Temperatures

#if 0   /* Now used for status bits */
  // add padding to 64-bit boundary
  for (uPaddingIndex = 0; uPaddingIndex < 2; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;
#endif

  *uResponseLength = sizeof(sGetSensorDataRespT);

  return XST_SUCCESS;
}

//=================================================================================
//  ReadFanSpeedRPM
//--------------------------------------------------------------------------------
//  This method reads the fan speed of a target fan in RPM
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes     OUT Read bytes (fan speed)
//  FanPage       IN  Desired fan
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadFanSpeedRPM(u16 * ReadBytes, unsigned FanPage, bool OpenSwitch){

  u16 WriteBytes[3];
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = FanPage;

  // open the I2C switch if required
  if (OpenSwitch)
  {
    stat += ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
  }

  // request a fan speed read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // read the fan speed
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, READ_FAN_SPEED_1_CMD, ReadBytes, 2);

  return stat;
}
//=================================================================================
//  ReadFanSpeedPWM
//--------------------------------------------------------------------------------
//  This method reads the PWM setting of a fan from the fan controller
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes     OUT Read bytes (fan speed)
//  FanPage       IN  Desired fan
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadFanSpeedPWM(u16 * ReadBytes, unsigned FanPage, bool OpenSwitch){

  u16 WriteBytes[3];
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = FanPage;

  // open the I2C switch if required
  if (OpenSwitch)
  {
    stat += ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
  }

  // request a fan speed read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // read the fan speed
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, MFR_READ_FAN_PWM_CMD, ReadBytes, 2);

  return stat;
}

//=================================================================================
//  GetAllFanSpeeds
//--------------------------------------------------------------------------------
//  This method reads the speeds of all the fans located on the SKARAB motherboard
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  Response  IN    Pointer to the response packet structure
//
//  Return
//  ------
//  None
//=================================================================================
void GetAllFanSpeeds(sGetSensorDataRespT *Response)
{
  u16 ReadBytes[3];
  u16 fanSpeed;
  int i;
  int ret;

  // list of fan pages
  unsigned fanPages[5] = {LEFT_FRONT_FAN_PAGE, LEFT_MIDDLE_FAN_PAGE,
    LEFT_BACK_FAN_PAGE, RIGHT_BACK_FAN_PAGE, FPGA_FAN};

  // read the speed of each fan on the motherboard
  // append the data to the response packet
  for (i = 0; i<5; i++){
    ret = ReadFanSpeedRPM(ReadBytes, fanPages[i], true);
    fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
    Response->uSensorData[i] = fanSpeed;
    if (ret){
      /* upon error - set error status bit */
      Response->uStatusBits[0] = Response->uStatusBits[0] | (1u << i);
    }
  }

  /*
  // LEFT FRONT FAN
  ReadFanSpeedRPM(ReadBytes, LEFT_FRONT_FAN_PAGE, true);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[0] = fanSpeed;

  // LEFT MIDDLE FAN
  ReadFanSpeedRPM(ReadBytes, LEFT_MIDDLE_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[1] = fanSpeed;

  // LEFT BACK FAN
  ReadFanSpeedRPM(ReadBytes, LEFT_BACK_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[2] = fanSpeed;

  // RIGHT BACK FAN
  ReadFanSpeedRPM(ReadBytes, RIGHT_BACK_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[3] = fanSpeed;

  // FPGA FAN
  ReadFanSpeedRPM(ReadBytes, FPGA_FAN, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[4] = fanSpeed;
   */
}
//=================================================================================
//  GetAllFanSpeedsPWM
//--------------------------------------------------------------------------------
//  This method reads the speeds of all the fans located on the SKARAB motherboard
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  Response  IN    Pointer to the response packet structure
//
//  Return
//  ------
//  None
//=================================================================================
void GetAllFanSpeedsPWM(sGetSensorDataRespT *Response)
{
  u16 ReadBytes[3];
  u16 fanSpeed;
  int i;
  int ret;

  // list of fan pages
  unsigned fanPages[5] = {LEFT_FRONT_FAN_PAGE, LEFT_MIDDLE_FAN_PAGE,
    LEFT_BACK_FAN_PAGE, RIGHT_BACK_FAN_PAGE, FPGA_FAN};

  // read the speed of each fan on the motherboard
  // append the data to the response packet
  for (i = 0; i<5; i++){
    ret = ReadFanSpeedPWM(ReadBytes, fanPages[i], true);
    fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
    Response->uSensorData[i+5] = fanSpeed; // offset of 5 to account for previous sensor data
    if (ret){
      /* upon error - set error status bit */
        Response->uStatusBits[0] = Response->uStatusBits[0] | (1u << (i+5));
    }
  }

  /*
  // LEFT FRONT FAN
  ReadFanSpeedPWM(ReadBytes, LEFT_FRONT_FAN_PAGE, true);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[5] = fanSpeed;

  // LEFT MIDDLE FAN
  ReadFanSpeedPWM(ReadBytes, LEFT_MIDDLE_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[6] = fanSpeed;

  // LEFT BACK FAN
  ReadFanSpeedPWM(ReadBytes, LEFT_BACK_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[7] = fanSpeed;

  // RIGHT BACK FAN
  ReadFanSpeedPWM(ReadBytes, RIGHT_BACK_FAN_PAGE, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[8] = fanSpeed;

  // FPGA FAN
  ReadFanSpeedPWM(ReadBytes, FPGA_FAN, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[9] = fanSpeed;
   */
}

//=================================================================================
//  ReadTemperature
//--------------------------------------------------------------------------------
//  This method reads a temperature from the MAX31785 Fan Controller
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes     OUT Read bytes (temperature)
//  TempSensorPage    IN  Desired temperature
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadTemperature(u16 * ReadBytes, unsigned TempSensorPage, bool OpenSwitch)
{
  u16 WriteBytes[3];
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = TempSensorPage;

  if (OpenSwitch)
  {
    stat += ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
  }

  // request temperature read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // read temperature
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, READ_TEMPERATURE_1_CMD, ReadBytes, 2);

  return stat;
}
//=================================================================================
//  ReadVoltageMonTemperature
//--------------------------------------------------------------------------------
//  This method reads the temperature of the UCD90120A Voltage Monitor
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes     OUT Read bytes (temperature)
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadVoltageMonTemperature(u16 * ReadBytes, bool OpenSwitch)
{
  int stat = 0;
  if (OpenSwitch)
  {
    stat += ConfigureSwitch(MONITOR_SWITCH_SELECT);
  }

  // read voltage monitor temperature
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_VMON_I2C_DEVICE_ADDRESS, READ_TEMPERATURE_1_CMD, ReadBytes, 2);

  return stat;
}
///=================================================================================
//  ReadCurrentMonTemperature
//--------------------------------------------------------------------------------
//  This method reads the temperature of the UCD90120A Current Monitor
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes     OUT Read bytes (temperature)
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadCurrentMonTemperature(u16 * ReadBytes, bool OpenSwitch)
{
  int stat = 0;
  if (OpenSwitch)
  {
    stat += ConfigureSwitch(MONITOR_SWITCH_SELECT);
  }

  // read current monitor temperature
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_CMON_I2C_DEVICE_ADDRESS, READ_TEMPERATURE_1_CMD, ReadBytes, 2);

  return stat;
}
//=================================================================================
//  GetAllTempSensors
//--------------------------------------------------------------------------------
//  This method reads all the temperature sensors on the SKARAB motherboard
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  Response  IN    Pointer to the response packet structure
//
//  Return
//  ------
//  None
//=================================================================================
void GetAllTempSensors(sGetSensorDataRespT *Response)
{
  u16 ReadBytes[3];
  u16 temperature;
  int i;
  int ret;

  unsigned TempSensorPages[6] = {INLET_TEMP_SENSOR_PAGE, OUTLET_TEMP_SENSOR_PAGE, FPGA_TEMP_DIODE_ADC_PAGE,
    FAN_CONT_TEMP_SENSOR_PAGE, VOLTAGE_MON_TEMP_SENSOR_PAGE,
    CURRENT_MON_TEMP_SENSOR_PAGE};


  for (i = 0; i<6; i++){
    if(i == 0){
      ret = ReadTemperature(ReadBytes, TempSensorPages[i], true);
    }
    else{
      if (TempSensorPages[i] == VOLTAGE_MON_TEMP_SENSOR_PAGE)
      {
        ret = ReadVoltageMonTemperature(ReadBytes, true);
      }
      else if (TempSensorPages[i] == CURRENT_MON_TEMP_SENSOR_PAGE){
        ret = ReadCurrentMonTemperature(ReadBytes, false);
      }
      else{
        ret = ReadTemperature(ReadBytes, TempSensorPages[i], false);
      }
    }
    temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
    Response->uSensorData[i+10] = temperature; // offset of 10 to account for previous sensor data
    if (ret){
      /* upon error - set error status bit */
      Response->uStatusBits[0] = Response->uStatusBits[0] | (1u << (i+10));
    }
  }

  /*
  // INLET TEMPERATURE SENSOR
  ReadTemperature(ReadBytes, INLET_TEMP_SENSOR_PAGE, true);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[10] = temperature;

  // OUTLET TEMPERATURE SENSOR
  ReadTemperature(ReadBytes, OUTLET_TEMP_SENSOR_PAGE, false);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[11] = temperature;

  // FPGA TEMPERATURE SENSOR
  ReadTemperature(ReadBytes, FPGA_TEMP_DIODE_ADC_PAGE, false);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[12] = temperature;

  // FAN CONTROLLER TEMPERATURE SENSOR
  ReadTemperature(ReadBytes, FAN_CONT_TEMP_SENSOR_PAGE, false);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[13] = temperature;

  // VOLTAGE MONITOR TEMPERATURE SENSOR
  ReadVoltageMonTemperature(ReadBytes, true);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[14] = temperature;

  // CURRENT MONITOR TEMPERATURE SENSOR
  ReadCurrentMonTemperature(ReadBytes, false);
  temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[15] = temperature;
   */

}
//=================================================================================
//  ReadVoltage
//--------------------------------------------------------------------------------
//  This method reads a voltage from the UCD90120A voltage monitor
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes OUT   Read bytes (voltage)
//  Voltage   IN    Desired voltage
//  OpenSwitch  IN    True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadVoltage(u16 * ReadBytes, unsigned Voltage, bool OpenSwitch)
{
  u16 WriteBytes[2];
  u16 VoltageScale;
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = Voltage; // select which voltage is to be read

  if (OpenSwitch)
  {
    stat += ConfigureSwitch(MONITOR_SWITCH_SELECT);
  }

  // request voltage sensor read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, UCD90120A_VMON_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // first read the voltage scaling factor
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_VMON_I2C_DEVICE_ADDRESS, VOUT_MODE_CMD, ReadBytes, 1);
  VoltageScale = ReadBytes[0];

  // read the voltage
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_VMON_I2C_DEVICE_ADDRESS, READ_VOUT_CMD, ReadBytes, 2);
  ReadBytes[2] = VoltageScale;

  return stat;
}
//=================================================================================
//  Read3V3Voltage
//--------------------------------------------------------------------------------
//  This method reads the +3V3Config voltage on revision 2 of the SKARAB board
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes OUT   Read bytes (voltage)
//  OpenSwitch  IN    True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int Read3V3Voltage(u16 * ReadBytes, bool OpenSwitch)
{
  u16 WriteBytes[2];
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = PLUS3V3CONFIG02_ADC_PAGE; // select which voltage is to be read

  if (OpenSwitch)
  {
    stat += ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
  }

  // request a voltage read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // read the voltage
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, READ_VOUT_CMD, ReadBytes, 2);

  return stat;
}
//=================================================================================
//  GetAllVoltages
//--------------------------------------------------------------------------------
//  This method reads the all voltages monitored by the UCD90120A Voltage Monitor
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  Response  IN    Pointer to the response packet structure
//
//  Return
//  ------
//  None
//=================================================================================
void GetAllVoltages(sGetSensorDataRespT *Response)
{
  u16 ReadBytes[3];
  u16 VoltageScaleFactor;
  u16 Voltage;
  int i;
  int ret;

  unsigned VoltagePages[13] = {P12V2_VOLTAGE_MON_PAGE, P12V_VOLTAGE_MON_PAGE, P5V_VOLTAGE_MON_PAGE,
    P3V3_VOLTAGE_MON_PAGE, P2V5_VOLTAGE_MON_PAGE, P1V8_VOLTAGE_MON_PAGE, P1V2_VOLTAGE_MON_PAGE,
    P1V0_VOLTAGE_MON_PAGE, P1V8_MGTVCCAUX_VOLTAGE_MON_PAGE, P1V0_MGTAVCC_VOLTAGE_MON_PAGE,
    P1V2_MGTAVTT_VOLTAGE_MON_PAGE, P5VAUX_VOLTAGE_MON_PAGE, PLUS3V3CONFIG02_ADC_PAGE};

  for (i = 0; i < 13; i++)
  {
    if (i == 0)
    {
      ret = ReadVoltage(ReadBytes, VoltagePages[i], true);
    }
    else if (VoltagePages[i] == PLUS3V3CONFIG02_ADC_PAGE)
    {
      ret = Read3V3Voltage(ReadBytes, true);
      ReadBytes[2] = 0; // dummy scale factor
    }
    else
    {
      ret = ReadVoltage(ReadBytes, VoltagePages[i], false);
    }

    Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
    VoltageScaleFactor = ReadBytes[2];
    Response->uSensorData[(i*3)+16] = Voltage;
    Response->uSensorData[(i*3)+17] = VoltageScaleFactor;
    Response->uSensorData[(i*3)+18] = VoltagePages[i];
    if (ret){
      /* upon error - set error status bit */
      Response->uStatusBits[1] = Response->uStatusBits[1] | (1u << i);
    }
  }

  /*// 12V2 Voltage
    ReadVoltage(ReadBytes, P12V2_VOLTAGE_MON_PAGE, true);
    Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
    VoltageScaleFactor = ReadBytes[2];
    Response->uSensorData[16] = Voltage;
    Response->uSensorData[17] = VoltageScaleFactor;
    Response->uSensorData[18] = P12V2_VOLTAGE_MON_PAGE;

  // 12V Voltage
  ReadVoltage(ReadBytes, P12V_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[19] = Voltage;
  Response->uSensorData[20] = VoltageScaleFactor;
  Response->uSensorData[21] = P12V_VOLTAGE_MON_PAGE;

  // 5V Voltage
  ReadVoltage(ReadBytes, P5V_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[22] = Voltage;
  Response->uSensorData[23] = VoltageScaleFactor;
  Response->uSensorData[24] = P5V_VOLTAGE_MON_PAGE;

  // 3V3 Voltage
  ReadVoltage(ReadBytes, P3V3_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[25] = Voltage;
  Response->uSensorData[26] = VoltageScaleFactor;
  Response->uSensorData[27] = P3V3_VOLTAGE_MON_PAGE;

  // 2V5 Voltage
  ReadVoltage(ReadBytes, P2V5_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[28] = Voltage;
  Response->uSensorData[29] = VoltageScaleFactor;
  Response->uSensorData[30] = P2V5_VOLTAGE_MON_PAGE;

  // 1V8 Voltage
  ReadVoltage(ReadBytes, P1V8_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[31] = Voltage;
  Response->uSensorData[32] = VoltageScaleFactor;
  Response->uSensorData[33] = P1V8_VOLTAGE_MON_PAGE;

  // 1V2 Voltage
  ReadVoltage(ReadBytes, P1V2_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[34] = Voltage;
  Response->uSensorData[35] = VoltageScaleFactor;
  Response->uSensorData[36] = P1V2_VOLTAGE_MON_PAGE;

  // 1V0 Voltage
  ReadVoltage(ReadBytes, P1V0_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[37] = Voltage;
  Response->uSensorData[38] = VoltageScaleFactor;
  Response->uSensorData[39] = P1V0_VOLTAGE_MON_PAGE;

  // 1V8_MGTVCCAUX Voltage
  ReadVoltage(ReadBytes, P1V8_MGTVCCAUX_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[40] = Voltage;
  Response->uSensorData[41] = VoltageScaleFactor;
  Response->uSensorData[42] = P1V8_MGTVCCAUX_VOLTAGE_MON_PAGE;

  // 1V0_MGTAVCC Voltage
  ReadVoltage(ReadBytes, P1V0_MGTAVCC_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[43] = Voltage;
  Response->uSensorData[44] = VoltageScaleFactor;
  Response->uSensorData[45] = P1V0_MGTAVCC_VOLTAGE_MON_PAGE;

  // 1V2_MGTAVTT Voltage
  ReadVoltage(ReadBytes, P1V2_MGTAVTT_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[46] = Voltage;
  Response->uSensorData[47] = VoltageScaleFactor;
  Response->uSensorData[48] = P1V2_MGTAVTT_VOLTAGE_MON_PAGE;

  // 3V3_CONFIG Voltage (rev 1 SKARAB)
  //ReadVoltage(ReadBytes, P3V3_CONFIG_VOLTAGE_MON_PAGE, false);
  //Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  //VoltageScaleFactor = ReadBytes[2];
  //Response->uSensorData[49] = Voltage;
  //Response->uSensorData[50] = VoltageScaleFactor;
  //Response->uSensorData[51] = P3V3_CONFIG_VOLTAGE_MON_PAGE;

  // 5V_AUX Voltage
  ReadVoltage(ReadBytes, P5VAUX_VOLTAGE_MON_PAGE, false);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  VoltageScaleFactor = ReadBytes[2];
  Response->uSensorData[49] = Voltage;
  Response->uSensorData[50] = VoltageScaleFactor;
  Response->uSensorData[51] = P5VAUX_VOLTAGE_MON_PAGE;

  //3V3 Config Voltage for SKARAB Rev2
  Read3V3Voltage(ReadBytes, true);
  Voltage = (ReadBytes[0] + (ReadBytes[1] << 8));
  Response->uSensorData[52] = Voltage;
  Response->uSensorData[53] = 0;
  //Response->uSensorData[51] = P3V3_CONFIG_VOLTAGE_MON_PAGE;
  Response->uSensorData[54] = PLUS3V3CONFIG02_ADC_PAGE;
  */
}
//=================================================================================
//  ReadCurrent
//--------------------------------------------------------------------------------
//  This method reads a current from the UCD90120A current monitor
//
//  --------- ---   -----------
//  Parameter Dir   Description
//  --------- ---   -----------
//  ReadBytes OUT   Read bytes (voltage)
//  Current   IN    Desired current
//  OpenSwitch  IN    True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  status
//=================================================================================
int ReadCurrent(u16 * ReadBytes, unsigned Current, bool OpenSwitch)
{
  u16 WriteBytes[2];
  u16 VoltageScale;
  int stat = 0;

  WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
  WriteBytes[1] = Current; // select which voltage is to be read

  if (OpenSwitch)
  {
    stat += ConfigureSwitch(MONITOR_SWITCH_SELECT);
  }

  // request voltage sensor read
  stat += WriteI2CBytes(MB_I2C_BUS_ID, UCD90120A_CMON_I2C_DEVICE_ADDRESS, WriteBytes, 2);

  // first read the voltage scaling factor
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_CMON_I2C_DEVICE_ADDRESS, VOUT_MODE_CMD, ReadBytes, 1);
  VoltageScale = ReadBytes[0];

  // read the voltage
  stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, UCD90120A_CMON_I2C_DEVICE_ADDRESS, READ_VOUT_CMD, ReadBytes, 2);
  ReadBytes[2] = VoltageScale;

  return stat;
}
//=================================================================================
//  GetAllCurrents
//--------------------------------------------------------------------------------
//  This method reads the all currents monitored by the UCD90120A Current Monitor
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  Response  IN    Pointer to the response packet structure
//
//  Return
//  ------
//  None
//=================================================================================
void GetAllCurrents(sGetSensorDataRespT *Response)
{
  u16 ReadBytes[3];
  u16 ScaleFactor;
  u16 Current;
  int i,j;
  int ret;

  unsigned CurrentPages[12] = {P12V2_CURRENT_MON_PAGE, P12V_CURRENT_MON_PAGE, P5V_CURRENT_MON_PAGE,
    P3V3_CURRENT_MON_PAGE, P2V5_CURRENT_MON_PAGE, P1V8_CURRENT_MON_PAGE, P1V2_CURRENT_MON_PAGE,
    P1V0_CURRENT_MON_PAGE, P1V8_MGTVCCAUX_CURRENT_MON_PAGE, P1V0_MGTAVCC_CURRENT_MON_PAGE,
    P1V2_MGTAVTT_CURRENT_MON_PAGE, P3V3_CONFIG_CURRENT_MON_PAGE};

  for (i = 0; i < 12; i++)
  {
    if (i == 0)
    {
      ret = ReadCurrent(ReadBytes, CurrentPages[i], true);
    }
    else
    {
      ret = ReadCurrent(ReadBytes, CurrentPages[i], false);
    }

    Current = (ReadBytes[0] + (ReadBytes[1] << 8));
    ScaleFactor = ReadBytes[2];
    Response->uSensorData[(i*3)+55] = Current;
    Response->uSensorData[(i*3)+56] = ScaleFactor;
    Response->uSensorData[(i*3)+57] = CurrentPages[i];

    j = i + 13;

    if (ret){
      /* upon error - set error status bit */
      if (j < 16) {
        Response->uStatusBits[1] = Response->uStatusBits[1] | (1u << j);
      } else {
        Response->uStatusBits[2] = Response->uStatusBits[2] | (1u << (j-16));
      }
    }
  }
  /*
  // 12V2 Current
  ReadCurrent(ReadBytes, P12V2_CURRENT_MON_PAGE, true);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[55] = Current;
  Response->uSensorData[56] = ScaleFactor;
  Response->uSensorData[57] = P12V2_CURRENT_MON_PAGE;

  // 12V Current
  ReadCurrent(ReadBytes, P12V_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[58] = Current;
  Response->uSensorData[59] = ScaleFactor;
  Response->uSensorData[60] = P12V_CURRENT_MON_PAGE;

  // 5V Current
  ReadCurrent(ReadBytes, P5V_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[61] = Current;
  Response->uSensorData[62] = ScaleFactor;
  Response->uSensorData[63] = P5V_CURRENT_MON_PAGE;

  // 3V3 Current
  ReadCurrent(ReadBytes, P3V3_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[64] = Current;
  Response->uSensorData[65] = ScaleFactor;
  Response->uSensorData[66] = P3V3_CURRENT_MON_PAGE;

  // 2V5 Current
  ReadCurrent(ReadBytes, P2V5_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[67] = Current;
  Response->uSensorData[68] = ScaleFactor;
  Response->uSensorData[69] = P2V5_CURRENT_MON_PAGE;

  // 1V8 Current
  ReadCurrent(ReadBytes, P1V8_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[70] = Current;
  Response->uSensorData[71] = ScaleFactor;
  Response->uSensorData[72] = P1V8_CURRENT_MON_PAGE;

  // 1V2 Current
  ReadCurrent(ReadBytes, P1V2_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[73] = Current;
  Response->uSensorData[74] = ScaleFactor;
  Response->uSensorData[75] = P1V2_CURRENT_MON_PAGE;

  // 1V0 Current
  ReadCurrent(ReadBytes, P1V0_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[76] = Current;
  Response->uSensorData[77] = ScaleFactor;
  Response->uSensorData[78] = P1V0_CURRENT_MON_PAGE;

  // 1V8_MGTVCCAUX Current
  ReadCurrent(ReadBytes, P1V8_MGTVCCAUX_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[79] = Current;
  Response->uSensorData[80] = ScaleFactor;
  Response->uSensorData[81] = P1V8_MGTVCCAUX_CURRENT_MON_PAGE;

  // 1V0_MGTAVCC Current
  ReadCurrent(ReadBytes, P1V0_MGTAVCC_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[82] = Current;
  Response->uSensorData[83] = ScaleFactor;
  Response->uSensorData[84] = P1V0_MGTAVCC_CURRENT_MON_PAGE;

  // 1V2_MGTAVTT Current
  ReadCurrent(ReadBytes, P1V2_MGTAVTT_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[85] = Current;
  Response->uSensorData[86] = ScaleFactor;
  Response->uSensorData[87] = P1V2_MGTAVTT_CURRENT_MON_PAGE;

  // 3V3_CONFIG Current
  ReadCurrent(ReadBytes, P3V3_CONFIG_CURRENT_MON_PAGE, false);
  Current = (ReadBytes[0] + (ReadBytes[1] << 8));
  ScaleFactor = ReadBytes[2];
  Response->uSensorData[88] = Current;
  Response->uSensorData[89] = ScaleFactor;
  Response->uSensorData[90] = P3V3_CONFIG_CURRENT_MON_PAGE;
  */
}
//=================================================================================
//	ReadMezzanineTemperature
//--------------------------------------------------------------------------------
//	This method reads a temperature from the MAX31785 Fan Controller
//
//	---------	---		-----------
//	Parameter	Dir		Description
//	---------	---		-----------
//  ReadBytes			OUT Read bytes (temperature)
//	TempSensorPage		IN	Desired messanine temperature
//	OpenSwitch			IN	True if the PCA9546 I2C switch needs to be opened first
//
//	Return
//	------
// 	status
//=================================================================================
int ReadMezzanineTemperature(u16 * ReadBytes, unsigned MezzaninePage, bool OpenSwitch)
{
	u16 WriteBytes[3];
	u16 Mez3WriteBytes[3];
  int stat = 0;

	WriteBytes[0] = PAGE_CMD; // command code for MAX31785 to select page to be controlled/read
	WriteBytes[1] = MezzaninePage;

	if (OpenSwitch)
	{
		stat += ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
	}

	// request temperature read
	stat += WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes, 2);

	// read mezzanine temperatures - QSFP card hardcoded to mezzanine 3
	if (MezzaninePage == MEZZANINE_3_TEMP_ADC_PAGE)
	{
		Mez3WriteBytes[0] = 0x7D;
		Mez3WriteBytes[1] = 0x00;

		// configure the QSFP to have it's temperature read
		stat += WriteI2CBytes(MEZZANINE_3_I2C_BUS_ID, STM_I2C_DEVICE_ADDRESS, Mez3WriteBytes, 2);

		//Delay(5000000);

		stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, READ_VOUT_CMD, ReadBytes, 2);
	}
	// read mezzanine temperatures - HMC cards hardcoded to mezzanine 0, 1 and 2
	if ((MezzaninePage == MEZZANINE_0_TEMP_ADC_PAGE) || (MezzaninePage == MEZZANINE_1_TEMP_ADC_PAGE) || (MezzaninePage == MEZZANINE_2_TEMP_ADC_PAGE))
	{
		stat += PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, READ_VOUT_CMD, ReadBytes, 2);

	}

  return stat;
}

//=================================================================================
//	GetAllMezzanineTempSensors
//--------------------------------------------------------------------------------
//	This method reads all the temperature sensors on the SKARAB motherboard
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	Response	IN		Pointer to the response packet structure
//
//	Return
//	------
//	None
//=================================================================================
void GetAllMezzanineTempSensors(sGetSensorDataRespT *Response)
{
		u16 ReadBytes[3];
		u16 temperature;
		int i;
    int ret;

		unsigned MezzanineSensorPages[3] = {MEZZANINE_0_TEMP_ADC_PAGE,
											MEZZANINE_1_TEMP_ADC_PAGE,
											MEZZANINE_2_TEMP_ADC_PAGE};


		for (i = 0; i<3; i++)
		{
			ret = ReadMezzanineTemperature(ReadBytes, MezzanineSensorPages[i], true);
			temperature = (ReadBytes[0] + (ReadBytes[1] << 8));
			Response->uSensorData[i+91] = temperature; // offset of 91 to account for previous sensor data

      if (ret){
        /* upon error - set error status bit */
        Response->uStatusBits[2] = Response->uStatusBits[2] | (1u << (i+9));
      }
		}
}

//=================================================================================
//	ConfigureSwitch
//--------------------------------------------------------------------------------
//  This method is used to configure the PCA9546 I2C Switch
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  SwitchSelection   IN  Desired switch configuration
//
//  Return
//  ------
//  None
//=================================================================================
int ConfigureSwitch(unsigned SwitchSelection){

  u16 WriteBytes[1];

  WriteBytes[0] = SwitchSelection;

  // configure the PCA9546 I2C switch to the desired selection
  return WriteI2CBytes(MB_I2C_BUS_ID, PCA9546_I2C_DEVICE_ADDRESS, WriteBytes, 1);
}

//=================================================================================
//  SetFanSpeedHandler
//--------------------------------------------------------------------------------
//  This method executes the SetFanSpeedHandler. This handler sets the speed of a selected fan
//  on the SKARAB Motherboard to a user defined value.
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  pCommand        IN  Pointer to command header
//  uCommandLength      IN  Length of command
//  uResponsePacketPtr    IN  Pointer to where response packet must be constructed
//  uResponseLength     OUT Length of payload of response packet
//
//  Return
//  ------
//  XST_SUCCESS if successful
//=================================================================================
int SetFanSpeedHandler(u8 * pCommand, u32 uCommandLength, u8 * uResponsePacketPtr, u32 * uResponseLength){

  sSetFanSpeedReqT *Command = (sSetFanSpeedReqT *) pCommand;
  sSetFanSpeedRespT *Response = (sSetFanSpeedRespT *) uResponsePacketPtr;
  u16 ReadBytes[2];
  u16 fanSpeed;
  u16 fanSpeedPWM;
  u8 uPaddingIndex;

  if (uCommandLength < sizeof(sSetFanSpeedReqT)){
    return XST_FAILURE;
  }

  // Create response packet
  Response->Header.uCommandType = Command->Header.uCommandType + 1;
  Response->Header.uSequenceNumber = Command->Header.uSequenceNumber;

  // set fan speed
  SetFanSpeed(Command->uFanPage, Command->uPWMSetting/100 , true);

  // check the previous fan speed (in rpm and pwm) at the time of setting
  ReadFanSpeedPWM(ReadBytes, Command->uFanPage, true);
  fanSpeedPWM = (ReadBytes[0] + (ReadBytes[1] << 8));
  ReadFanSpeedRPM(ReadBytes, Command->uFanPage, false);
  fanSpeed = (ReadBytes[0] + (ReadBytes[1] << 8));

  // add current fan speed reads to response
  Response->uNewFanSpeedPWM = fanSpeedPWM;
  Response->uNewFanSpeedRPM = fanSpeed;

  // pad response
  for (uPaddingIndex = 0; uPaddingIndex < 7; uPaddingIndex++)
    Response->uPadding[uPaddingIndex] = 0;

  *uResponseLength = sizeof(sSetFanSpeedRespT);

  return XST_SUCCESS;
}
//=================================================================================
//  SetFanSpeed
//--------------------------------------------------------------------------------
//  This method sets the fan speed of a target fan given a PWM setting
//
//  Parameter Dir   Description
//  --------- ---   -----------
//  FanPage       IN  Desired fan
//  PWMPercentage   IN  Fan PWM setting
//  OpenSwitch      IN  True if the PCA9546 I2C switch needs to be opened first
//
//  Return
//  ------
//  None
//=================================================================================
void SetFanSpeed(unsigned FanPage, float PWMPercentage, bool OpenSwitch)
{
  u16 WriteBytes_1[2];
  u16 WriteBytes_2[3];
  unsigned FANPWMDirect;

  if (OpenSwitch)
  {
    ConfigureSwitch(FAN_CONT_SWTICH_SELECT);
  }

  WriteBytes_1[0] = PAGE_CMD; // command code for I2C
  WriteBytes_1[1] = FanPage;

  // prepare for a fan speed change
  WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes_1, 2);

  FANPWMDirect = (unsigned)(PWMPercentage * 100.0f);

  // prepare fan speed value for writing to controller
  WriteBytes_2[0] = FAN_COMMAND_1_CMD;
  WriteBytes_2[1] = FANPWMDirect & 0xFF;
  WriteBytes_2[2] = (FANPWMDirect >> 8) & 0xFF;

  // set the fan speed
  WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, WriteBytes_2, 3);
}
//=================================================================================
//  GetAllHMCDieTemperatures
//--------------------------------------------------------------------------------
//	This method reads all the HMC Die temperature sensors on the SKARAB motherboard
//
//	Parameter	Dir		Description
//	---------	---		-----------
//	Response	IN		Pointer to the response packet structure
//
//	Return
//	------
//	None
//=================================================================================
void GetAllHMCDieTemperatures(sGetSensorDataRespT *Response)
{

	u16 ReadBytes[4] = {1,2,3,4};
	u16 uWriteBytes[8];
  u16 uReadAddress[4];
	int i;
  int stat;

	unsigned HMC_Mezzanine_Sites[3] = {HMC_Mezzanine_Site_1,
									   HMC_Mezzanine_Site_2,
									   HMC_Mezzanine_Site_3};

  /* write the following 8 bytes of data to the I2C bus */
  uWriteBytes[0] = (HMC_Temperature_Write_Reg >> 24) & 0xff;     /* MSB of HMC reg addr */
  uWriteBytes[1] = (HMC_Temperature_Write_Reg >> 16) & 0xff;
  uWriteBytes[2] = (HMC_Temperature_Write_Reg >>  8) & 0xff;
  uWriteBytes[3] = (HMC_Temperature_Write_Reg      ) & 0xff;     /* LSB of HMC reg addr */

  uWriteBytes[4] = (HMC_Temperature_Write_Command >> 24) & 0xff;        /* MSB of data word to be written */
  uWriteBytes[5] = (HMC_Temperature_Write_Command >> 16) & 0xff;
  uWriteBytes[6] = (HMC_Temperature_Write_Command >>  8) & 0xff;
  uWriteBytes[7] = (HMC_Temperature_Write_Command      ) & 0xff;        /* LSB of data word to be written */

  for (i = 0; i<3; i++){
    /* Determine if the hmc cores have been compiled into the firmware */
    if (get_mezz_firmware_type(i) == MEZ_FIRMW_TYPE_HMC_R1000_0005){

      stat = WriteI2CBytes(HMC_Mezzanine_Sites[i], HMC_I2C_Address, uWriteBytes, 8);

      uReadAddress[0] = (HMC_Die_Temperature_Reg >> 24) & 0xff;
      uReadAddress[1] = (HMC_Die_Temperature_Reg >> 16) & 0xff;
      uReadAddress[2] = (HMC_Die_Temperature_Reg >>  8) & 0xff;
      uReadAddress[3] = (HMC_Die_Temperature_Reg      ) & 0xff;

      stat += HMCReadI2CBytes(HMC_Mezzanine_Sites[i], HMC_I2C_Address, uReadAddress, ReadBytes);

      Response->uSensorData[(i*4)+94] = ReadBytes[0]; // offset of 94 to account for previous sensor data
      Response->uSensorData[(i*4)+95] = ReadBytes[1];
      Response->uSensorData[(i*4)+96] = ReadBytes[2];
      Response->uSensorData[(i*4)+97] = ReadBytes[3];

      if (stat){
        Response->uStatusBits[2] = Response->uStatusBits[2] | (1u << (i+13));
      }
    } else {
      Response->uSensorData[(i*4)+94] = 0xff; // offset of 94 to account for previous sensor data
      Response->uSensorData[(i*4)+95] = 0xee;
      Response->uSensorData[(i*4)+96] = 0xdd;
      Response->uSensorData[(i*4)+97] = 0xcc;

      /* should we set error bits for hmc's not present? */
      Response->uStatusBits[2] = Response->uStatusBits[2] | (1u << (i+13));

    }
  }
}
// next offset = 106
