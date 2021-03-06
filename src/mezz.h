#ifndef _MEZZ_H_
#define _MEZZ_H_

#include <xil_types.h>

#include "qsfp.h"
#include "adc.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define MEZ_BOARD_TYPE_OPEN               0x0
#define MEZ_BOARD_TYPE_UNKNOWN            0x1
#define MEZ_BOARD_TYPE_QSPF               0x2
#define MEZ_BOARD_TYPE_QSFP_PHY           0x3
#define MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2 0x4
#define MEZ_BOARD_TYPE_HMC_R1000_0005     0x5
#endif

#define MEZZ_MAGIC  0xA5A5A5A5

typedef enum {
  MEZ_BOARD_TYPE_OPEN,
  MEZ_BOARD_TYPE_UNKNOWN,
  MEZ_BOARD_TYPE_QSFP,
  MEZ_BOARD_TYPE_QSFP_PHY,
  MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2,
  MEZ_BOARD_TYPE_HMC_R1000_0005
} MezzHWType;

typedef enum {
  MEZ_FIRMW_TYPE_OPEN,
  MEZ_FIRMW_TYPE_UNKNOWN,
  MEZ_FIRMW_TYPE_QSFP,
  MEZ_FIRMW_TYPE_QSFP_PHY,
  MEZ_FIRMW_TYPE_SKARAB_ADC32RF45X2,
  MEZ_FIRMW_TYPE_HMC_R1000_0005
} MezzFirmwType;

  typedef union {
    //void none;
    struct sQSFPObject QSFPContext;
    struct sAdcObject AdcContext;
    //struct sHmcObject HmcContext;
  } mezz_obj;

struct sMezzObject {
  u32 m_magic;
  u8 m_site;
  u8 m_type;        /* describes the hardware */
#define FIRMW_SUPPORT_FALSE   0
#define FIRMW_SUPPORT_TRUE    1
  u8 m_firmw_support;  /* has support been compiled into firmware */
  u8 m_allow_init;     /* set if both hardware AND firmware suppported */

  mezz_obj m_obj;
};

struct sMezzObject *lookup_mezz_handle_by_site(u8 mezz_site);
struct sMezzObject *init_mezz_location(u8 mezz_site);
void run_qsfp_mezz_mgmt(void);
void run_adc_mezz_mgmt(void);

typedef enum{
  ADC_MEZZ_SM_PAUSE_OP,
  ADC_MEZZ_SM_RESUME_OP,
  ADC_MEZZ_SM_RESET_OP
} AdcMezzSMOperation;

void adc_mezz_sm_control(u8 mezz_site, AdcMezzSMOperation op);

u8 is_adc_mezz(u8 mezz_site);

MezzFirmwType get_mezz_firmware_type(u8 mezz_site);
MezzHWType read_mezz_type_id(u8 mezz_site);

#ifdef __cplusplus
}
#endif
#endif
