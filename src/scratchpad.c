/*
 * Memory with persistent state between FPGA reconfigures is required to hold
 * programtic state upon a reconfigure (not between resets).
 * The fan controller MAX31785 chip on the SKARAB motherboard has RAM which
 * can be accessed and used for this purpose. This is done by writing to the
 * chip's registers (chose one's which do not influence operation). For this
 * purpose we will choose registers associated with the MFR_LOCATION PMBus
 * command, which gives us 8 bytes of memory to use.
 * We will not issue the "write-to-flash" command. When the FPGA reconfigures,
 * this register can be read and states will persist. Upon reset, the states
 * will be cleared. We do not want to write to FLASH as not to add to the
 * FLASH wear-out.
 * The 8 bytes of memory will be allocated as follows:
 * byte[0]    => counter for reconfigurations caused by HMC timeouts
 * byte[1]    => counter for reconfigurations caused by DHCP timeouts
 * byte[2-8]  => reserved for future use
 */

#include <xparameters.h>
#include <xil_assert.h>
#include <xstatus.h>

#include "sensors.h"
#include "i2c_master.h"
#include "print.h"
#include "scratchpad.h"

#define MFR_LOCATION_CMD 0x9c
#define MFR_LOCATION_LEN 8

/*
 * Check whether this memory has been set up for our use. The reason we need to
 * do this is because the 8 bytes of memory associated with the MFR_LOCATION
 * command have a default value, which may interfere with our check upon first
 * use of this feature. This function is thus basically used to check whether
 * this is the first time this software feature is running on the board.
 */

typedef enum {
  PMEM_CLEAR_ALL,
  PMEM_WRITE_BYTE,
  PMEM_READ_BYTE,
  PMEM_CHECK_REG,
  SKIP  /* this is used to skip the switch-case statement upon an earlier error,
           allows us to have one exit point from function */
} tPMemOperation;

/* local unction prototype */
static tPMemReturn PersistentMemory(tPMemOperation op, tPMemByteIndex byte_index, u8 *byte_data);

/* wrappers */

tPMemReturn PersistentMemory_Check(void){
  return PersistentMemory(PMEM_CHECK_REG, 0xff, NULL);
}

tPMemReturn PersistentMemory_ReadByte(tPMemByteIndex byte_index, u8 *read_byte_data){
  return PersistentMemory(PMEM_READ_BYTE, byte_index, read_byte_data);
}

tPMemReturn PersistentMemory_WriteByte(tPMemByteIndex byte_index, u8 write_byte_data){
  return PersistentMemory(PMEM_WRITE_BYTE, byte_index, &write_byte_data);
}

tPMemReturn PersistentMemory_Clear(void){
  return PersistentMemory(PMEM_CLEAR_ALL, 0xff, NULL);
}

/* 
 * Justification for the structure and length of this function:
 * In an attempt to minimize i2c operations or duplicate code - have one
 * function which accepts desired operation since many common steps between
 * operations. In that way the complexity lives in this function and allows
 * us to create wrapper functions for each operation.
 */
static tPMemReturn PersistentMemory(tPMemOperation op, tPMemByteIndex byte_index, u8 *byte_data){
  const u16 wr_bytes_page[2] = {PAGE_CMD, 255};
  const u16 MFR_LOCATION_default_values[8] = {0x30, 0x31, 0x30, 0x31, 0x30,
    0x31, 0x30, 0x31};
  u16 rd_bytes[8] = {0x0};
  u16 wr_bytes_data[MFR_LOCATION_LEN + 1] = {0};
  u8 index;
  u8 ret = PMEM_RETURN_OK;

  /* 
   * operations which require sanity checks:
   * this would be more compact with an if-statement, but switch allows
   * compiler to check that no op's are missed (-Wswitch-enum) and
   * default catches invalid op's!
   */
  switch (op){
    case PMEM_CLEAR_ALL:
    case PMEM_CHECK_REG:
      break;

    case PMEM_WRITE_BYTE:
    case PMEM_READ_BYTE:
      Xil_AssertNonvoid((byte_index >= 0) && (byte_index < PMEM_INDEX_MAX) && (NULL != byte_data));
      break;

    case SKIP:
      break;

    default:
      /* should never reach here - so always assert */
      Xil_AssertNonvoidAlways();
      break;
  }

  ConfigureSwitch(FAN_CONT_SWTICH_SELECT);

  /* minor FIXME: the next line contains a cast to (u16 *) to silence compiler -
   * strictly speaking, we should change the function definition to accept a
   * const u16 * since this data would not be changed. This should apply to all
   * other functions. */
  ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS, (u16 *) wr_bytes_page, 2);
  if (ret != XST_SUCCESS){
    ret = PMEM_RETURN_ERROR;
    op = SKIP;
  }

  /* operations which require read */
  switch (op){
    case PMEM_CHECK_REG:
    case PMEM_WRITE_BYTE:
    case PMEM_READ_BYTE:
      ret = PMBusReadI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS,
          MFR_LOCATION_CMD, rd_bytes, MFR_LOCATION_LEN);
      if (ret != XST_SUCCESS){
        ret = PMEM_RETURN_ERROR;
        op = SKIP;
      }
      break;

    case PMEM_CLEAR_ALL:
    case SKIP:
    default:
      /* should never reach default */
      /* previous assert should catch this */
      break;
  }

  wr_bytes_data[0] = MFR_LOCATION_CMD;

  /* final steps depending on operation */
  switch(op){ 
    case PMEM_WRITE_BYTE:
      /* copy register and set relevant byte */
      for (index = 0; index < MFR_LOCATION_LEN; index++){
        wr_bytes_data[index + 1] = rd_bytes[index];
      }
      wr_bytes_data[byte_index + 1] = *byte_data;
      /* Note: fall-through i.e. no break */

    case PMEM_CLEAR_ALL:
      /* wr_bytes_data already initialized with zeros */
      ret = WriteI2CBytes(MB_I2C_BUS_ID, MAX31785_I2C_DEVICE_ADDRESS,
          wr_bytes_data, 9);
      if (ret != XST_SUCCESS){
        ret = PMEM_RETURN_ERROR;
        op = SKIP;
      }
      break;

    case PMEM_CHECK_REG:
      for (index = 0; index < MFR_LOCATION_LEN; index++){
        if (rd_bytes[index] != MFR_LOCATION_default_values[index]){
          ret = PMEM_RETURN_NON_DEFAULT;
          break;
        } else {
          ret = PMEM_RETURN_DEFAULT;
        }
      }
      break;

    case PMEM_READ_BYTE:
      *byte_data = rd_bytes[byte_index];
      break;

    case SKIP:
    default:
      /* should never reach default */
      /* previous assert should catch this */
      break;
  }

  ConfigureSwitch(0);
  return ret;
}
