#ifndef _SCRATCHPAD_H_
#define _SCRATCHPAD_H_

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PMEM_RETURN_OK = 0,
  PMEM_RETURN_ERROR,
  PMEM_RETURN_DEFAULT,
  PMEM_RETURN_NON_DEFAULT
} tPMemReturn;

/* indexing map of persistent memory usage */
typedef enum {
  /* start of byte-block 0 */
  LOG_LEVEL_STARTUP_INDEX = 0,  /* log-level, if changed via cli, is retained across fpga reconfigures or mb resets - not power cycles */
  DHCP_RECONFIG_COUNT_INDEX,    /* deprecated - count the DHCP triggered reconfigurations */
  DHCP_CACHED_IP_STATE_INDEX,   /* indicates whether ip lease has been cached previously */
  DHCP_CACHED_IP_OCT0_INDEX,    /* next four indices store ip between reconfigs */
  DHCP_CACHED_IP_OCT1_INDEX,    /* cached ip => {oct1}.{oct2}.{oct3}.{oct4} */
  DHCP_CACHED_IP_OCT2_INDEX,
  DHCP_CACHED_IP_OCT3_INDEX,
  LINK_RECONFIG_COUNT_INDEX,    /* count the "no link" triggered reconfurations */
  /* start of byte-block 1 */
  DHCP_CACHED_MASK_OCT0_INDEX,  /* next four indices store netmask between reconfigs */
  DHCP_CACHED_MASK_OCT1_INDEX,  /* cached netmask => {oct1}.{oct2}.{oct3}.{oct4} */
  DHCP_CACHED_MASK_OCT2_INDEX,
  DHCP_CACHED_MASK_OCT3_INDEX,
  DHCP_CACHED_GW_OCT0_INDEX,    /* next four indices store gateway between reconfigs */
  DHCP_CACHED_GW_OCT1_INDEX,    /* cached gateway => {oct1}.{oct2}.{oct3}.{oct4} */
  DHCP_CACHED_GW_OCT2_INDEX,
  DHCP_CACHED_GW_OCT3_INDEX,
  /* start of byte-block 2 */
  HMC_RECONFIG_TOTAL_COUNT_INDEX,   /* count the total HMC triggered reconfigurations */
  HMC_RECONFIG_HMC0_COUNT_INDEX,    /* count the HMC 0 triggered reconfigurations */
  HMC_RECONFIG_HMC1_COUNT_INDEX,    /* count the HMC 1 triggered reconfigurations */
  HMC_RECONFIG_HMC2_COUNT_INDEX,    /* count the HMC 2 triggered reconfigurations */
  HMC_RECONFIG_HMC3_COUNT_INDEX,    /* count the HMC 3 triggered reconfigurations */
  PMEM_BLOCK2_BYTE5_INDEX,          /* unused */
  PMEM_BLOCK2_BYTE6_INDEX,          /* unused */
  AUX_SKARAB_FLAGS_INDEX,           /* bit0 - last reconfig location (0 = flash, 1 = sdram); bit1 - lock bit for bit0;
                                       the reset are unused for now */
  PMEM_INDEX_MAX = 24
  /* NOTE: 24 byte (3 x 8-byte) storage block implemented */
} tPMemByteIndex;

tPMemReturn PersistentMemory_Check(void);
tPMemReturn PersistentMemory_WriteByte(tPMemByteIndex byte_index, u8 byte_data);
tPMemReturn PersistentMemory_ReadByte(tPMemByteIndex byte_index, u8 *read_byte_data);
tPMemReturn PersistentMemory_Clear(void);

#ifdef __cplusplus
}
#endif
#endif /*_SCRATCHPAD_H_*/
