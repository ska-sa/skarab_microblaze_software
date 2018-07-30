#ifndef _QSFP_H_
#define _QSFP_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /* init states */
  //QSFP_STATE_INIT_RESET=0,
  QSFP_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE,
  QSFP_STATE_INIT_BOOTLOADER_VERSION_READ_MODE,
  QSFP_STATE_INIT_BOOTLOADER_MODE,
  QSFP_STATE_INIT_STARTING_APPLICATION_MODE,
  QSFP_STATE_INIT_BOOTLOADER_PROGRAMMING_MODE,
  QSFP_STATE_INIT_APPLICATION_MODE
} typeQSFPInitState;

typedef enum {
  /* application states */
  QSFP_STATE_APP_UPDATING_TX_LEDS,
  QSFP_STATE_APP_UPDATING_RX_LEDS,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_WR,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_0_RD,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_WR,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_1_RD,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_WR,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_2_RD,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_WR,
  QSFP_STATE_APP_UPDATING_MOD_PRSNT_3_RD
} typeQSFPAppState;


struct sQSFPObject {
  /* uQSFPMezzanineLocation; */
  unsigned int uWaitCount;
  typeQSFPInitState InitState;
  typeQSFPAppState AppState;
  /* uQSFPBootloaderVersionMajor; */
  /* uQSFPBootloaderVersionMinor; */
};


u8 uQSFPInit(struct sQSFPObject *pQSFPObject);
u8 QSFPStateMachine(struct sQSFPObject *pQSFPObject);

void uQSFPResetInit(void);
void uQSFPResetApp(void);
void uQSFPStateMachineResume(void);
void uQSFPStateMachinePause(void);

#ifdef __cplusplus
}
#endif

#endif /*_QSFP_H_*/
