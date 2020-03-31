#include <xil_types.h>

#include "init.h"
#include "logging.h"
#include "scratchpad.h"

u8 init_persistent_memory_setup(void){
  u8 pmem_status;

  pmem_status = PersistentMemory_Check();

  if (pmem_status == PMEM_RETURN_DEFAULT){
    /* if the memory in it's default state, we're safe to proceed and clear
     * the memory. This is an indication that 1) the FLASH location has not been
     * set for the board and following this 2) the board has been hard power
     * cycled. */
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "INIT [..] persistent memory contains default value. Clearing...\r\n");
    pmem_status = PersistentMemory_Clear();
  }

  /* catch any errors */
  if (pmem_status == PMEM_RETURN_ERROR){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "INIT [..] error setting up persistent memory.\r\n");
  }

  return pmem_status;
}



