#include <xil_types.h>
#include <xstatus.h>

#include "one_wire.h"
#include "id.h"
#include "logging.h"
#include "constant_defs.h"

//=================================================================================
//  get_skarab_serial
//---------------------------------------------------------------------------------
//  Get the SKARAB serial number stored in DS2433 One Wire EEPROM
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  sk_buffer       OUT   User provided char buffer (shall be at least 4 bytes wide)
//  sk_length       IN    size of the user specified sk_buffer in bytes
//
//  Return
//  ------
//  XST_SUCCESS or XST_FAILURE
//=================================================================================
u8 get_skarab_serial(u8 *sk_buffer, u8 sk_length){
  static u16 skarab_serial[ID_SK_SERIAL_LEN] = {0xff}; /* bytes stored as halfwords */
  u16 uRom[8];
  u16 uRomCRC;
  u8 len, i, ret = XST_SUCCESS;

  /* if first byte set to 0xff, assume that we have not yet read the serial from
   * the one wire ROM otherwise use the cached value */
  if (skarab_serial[0] == 0xff){
    ret = OneWireReadRom(uRom, MB_ONE_WIRE_PORT);
    if (ret == XST_SUCCESS){
      // GT 05/06/2015 CHECK CRC OF ROM TO CONFIRM OK
      uRomCRC = OneWireCrc8(& uRom[0], 0x7);
      if (uRomCRC != uRom[7]){
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MB 1-wire CRC incorrect.\r\n");
        skarab_serial[0] = 0xFF;
        skarab_serial[1] = 0xFF;
        skarab_serial[2] = 0xFF;
        skarab_serial[3] = 0xFF;
      } else {
        // Read the serial number
        ret = DS2433ReadMem(uRom, 0, skarab_serial, 4, 0, 0, MB_ONE_WIRE_PORT);

        if (ret == XST_FAILURE){
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read skarab serial number.\r\n");
          skarab_serial[0] = 0xFF;
          skarab_serial[1] = 0xFF;
          skarab_serial[2] = 0xFF;
          skarab_serial[3] = 0xFF;
        }
      }
    } else {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read skarab serial number.\r\n");
      skarab_serial[0] = 0xFF;
      skarab_serial[1] = 0xFF;
      skarab_serial[2] = 0xFF;
      skarab_serial[3] = 0xFF;
    }

    // GT 04/06/2015 BASIC SANITY CHECK, IF FAILS TRY TO READ ONE MORE TIME
    if (skarab_serial[0] != 0x50){    /* CHECK FOR 'P' */
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Trying again to read serial number.\r\n");

      ret = OneWireReadRom(uRom, MB_ONE_WIRE_PORT);

      if (ret == XST_SUCCESS){
        // GT 05/06/2015 CHECK CRC OF ROM TO CONFIRM OK
        uRomCRC = OneWireCrc8(& uRom[0], 0x7);
        if (uRomCRC != uRom[7]){
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MB 1-wire CRC incorrect.\r\n");
          skarab_serial[0] = 0xFF;
          skarab_serial[1] = 0xFF;
          skarab_serial[2] = 0xFF;
          skarab_serial[3] = 0xFF;
        } else {
          // Read the serial number
          ret = DS2433ReadMem(uRom, 0, skarab_serial, 4, 0, 0, MB_ONE_WIRE_PORT);

          if (ret == XST_FAILURE){
            log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read skarab serial number.\r\n");
            skarab_serial[0] = 0xFF;
            skarab_serial[1] = 0xFF;
            skarab_serial[2] = 0xFF;
            skarab_serial[3] = 0xFF;
          }
        }
      } else {
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read skarab serial number.\r\n");
        skarab_serial[0] = 0xFF;
        skarab_serial[1] = 0xFF;
        skarab_serial[2] = 0xFF;
        skarab_serial[3] = 0xFF;
      }
    }
  }

  /* ensure the read is bound - prevent array index overflow */
  len = (sk_length <= ID_SK_SERIAL_LEN ? sk_length : ID_SK_SERIAL_LEN);

  for (i = 0; i < len; i++){
    sk_buffer[i] = (u8) (skarab_serial[i] & 0xff);
  }

  return ret;
}

//=================================================================================
//  get_peralex_serial
//---------------------------------------------------------------------------------
//  Get the Peralex serial number stored in DS2433 One Wire EEPROM
//
//  Parameter       Dir   Description
//  ---------       ---   -----------
//  px_buffer       OUT   User provided char buffer (shall be at least 3 bytes wide)
//  px_length       IN    size of the user specified px_buffer in bytes
//
//  Return
//  ------
//  XST_SUCCESS or XST_FAILURE
//=================================================================================
u8 get_peralex_serial(u8 *px_buffer, u8 px_length){
  static u16 peralex_serial[ID_PX_SERIAL_LEN] = {0xff}; /* bytes stored as halfwords */
  u16 uRom[8];
  u16 uRomCRC;
  u8 len, i, ret = XST_SUCCESS;

  /* if first byte set to 0xff, assume that we have not yet read the serial from
   * the one wire ROM otherwise use the cached value */
  if (peralex_serial[0] == 0xff){
    ret = OneWireReadRom(uRom, MB_ONE_WIRE_PORT);
    if (ret == XST_SUCCESS){
      // GT 05/06/2015 CHECK CRC OF ROM TO CONFIRM OK
      uRomCRC = OneWireCrc8(& uRom[0], 0x7);
      if (uRomCRC != uRom[7]){
        log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "MB 1-wire CRC incorrect.\r\n");
        peralex_serial[0] = 0xFF;
        peralex_serial[1] = 0xFF;
        peralex_serial[2] = 0xFF;
      } else {
        // Read the serial number
        ret = DS2433ReadMem(uRom, 0, peralex_serial, 3, 7, 0, MB_ONE_WIRE_PORT);

        if (ret == XST_FAILURE){
          log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read motherboard serial number.\r\n");
          peralex_serial[0] = 0xFF;
          peralex_serial[1] = 0xFF;
          peralex_serial[2] = 0xFF;
        }
      }
    } else {
      log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Failed to read motherboard serial number.\r\n");
      peralex_serial[0] = 0xFF;
      peralex_serial[1] = 0xFF;
      peralex_serial[2] = 0xFF;
    }
  }

  /* ensure the read is bound - prevent array index overflow */
  len = (px_length <= ID_PX_SERIAL_LEN ? px_length : ID_PX_SERIAL_LEN);

  for (i = 0; i < len; i++){
    px_buffer[i] = (u8) (peralex_serial[i] & 0xff);
  }

  return ret;
}
