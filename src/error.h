#ifndef _error_h_
#define _error_h_

#include <xil_types.h>

#ifdef __cplusplus
extern "C"{
#endif

#define ERROR_AXI_DATA_BUS  0x1u

void set_error_flag(u8 errno);
u8 read_and_clear_error_flag(void);

#ifdef __cplusplus
}
#endif

#endif
