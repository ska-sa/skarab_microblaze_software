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
  HMC_RECONFIG_COUNT_INDEX = 0, /* count the HMC triggered reconfigurations */
  DHCP_RECONFIG_COUNT_INDEX,    /* count the DHCP triggered reconfigurations */
  DHCP_CACHED_IP_STATE_INDEX,   /* indicates whether ip has been cached previously */
  DHCP_CACHED_IP_OCT0_INDEX,    /* next four indices store ip between reconfigs */
  DHCP_CACHED_IP_OCT1_INDEX,    /* cached ip => {oct1}.{oct2}.{oct3}.{oct4} */
  DHCP_CACHED_IP_OCT2_INDEX,
  DHCP_CACHED_IP_OCT3_INDEX,
  PMEM_RESERVED,
  PMEM_INDEX_MAX = 8
  /* NOTE: only 8-byte storage block implemented */
} tPMemByteIndex;

tPMemReturn PersistentMemory_Check(void);
tPMemReturn PersistentMemory_WriteByte(tPMemByteIndex byte_index, u8 byte_data);
tPMemReturn PersistentMemory_ReadByte(tPMemByteIndex byte_index, u8 *read_byte_data);
tPMemReturn PersistentMemory_Clear(void);

#ifdef __cplusplus
}
#endif
#endif /*_SCRATCHPAD_H_*/
