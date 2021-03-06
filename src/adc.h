#ifndef _ADC_H_
#define _ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ADC_STATE_INIT_BOOTLOADER_VERSION_WRITE_MODE,
  ADC_STATE_INIT_BOOTLOADER_VERSION_READ_MODE,
  ADC_STATE_INIT_BOOTLOADER_MODE,
  ADC_STATE_INIT_STARTING_APPLICATION_MODE,
  ADC_STATE_INIT_BOOTLOADER_PROGRAMMING_MODE,
  ADC_STATE_INIT_APPLICATION_MODE
} typeAdcInitState;

/*
 * The application state does nothing currently, but mke provision for
 * application state expansion in future in the state machine.
 */
typedef enum {
  /* application states */
  ADC_STATE_APP_DO_NOTHING
} typeAdcAppState;

#define ADC_MAGIC 0xadc0adc0
struct sAdcObject{
  int AdcMagic;
  unsigned int uMezzLocation;
  unsigned int uWaitCount;
  typeAdcInitState InitState;
  typeAdcAppState AppState;
  /* uAdcBootloaderVersionMajor; */
  /* uAdcBootloaderVersionMinor; */
  unsigned int StateMachinePause;
};


u8 AdcInit(struct sAdcObject *pAdcObject, unsigned int mezz_site);
u8 AdcStateMachine(struct sAdcObject *pAdcObject);

void uAdcStateMachineReset(struct sAdcObject *pAdcObject);
void uAdcStateMachinePause(struct sAdcObject *pAdcObject);
void uAdcStateMachineResume(struct sAdcObject *pAdcObject);

#ifdef __cplusplus
}
#endif
#endif
