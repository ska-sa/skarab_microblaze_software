#ifndef _SCRATCHPAD_H_
#define _SCRATCHPAD_H_

typedef enum {
  PMEM_RETURN_OK = 0,
  PMEM_RETURN_ERROR,
  PMEM_RETURN_DEFAULT,
  PMEM_RETURN_NON_DEFAULT
} tPMemReturn;

typedef enum {
  HMC_RECONFIG_COUNT_BYTE   = 0,
  DHCP_RECONFIG_COUNT_BYTE  = 1,
} tPMemByteIndex;

tPMemReturn PersistentMemory_Check(void);
tPMemReturn PersistentMemory_WriteByte(tPMemByteIndex byte_index, u8 byte_data);
tPMemReturn PersistentMemory_ReadByte(tPMemByteIndex byte_index, u8 *read_byte_data);
tPMemReturn PersistentMemory_Clear(void);

#endif /*_SCRATCHPAD_H_*/
