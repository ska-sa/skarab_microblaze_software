#ifndef _MEZZ_H_
#define _MEZZ_H_

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MEZ_BOARD_TYPE_OPEN               0x0
#define MEZ_BOARD_TYPE_UNKNOWN            0x1
#define MEZ_BOARD_TYPE_QSPF               0x2
#define MEZ_BOARD_TYPE_QSFP_PHY           0x3
#define MEZ_BOARD_TYPE_SKARAB_ADC32RF45X2 0x4
#define MEZ_BOARD_TYPE_HMC_R1000_0005     0x5

struct sMezzObject {
  u8 m_slot;
  u8 m_type;

  union {
    struct sQSFPObject QSFPContext;
    struct sAdcObject AdcContext;
    //struct sHmcObject HmcContext;
  } m_sel;
};



#ifdef __cplusplus
}
#endif
#endif
