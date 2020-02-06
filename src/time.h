/*
   rvw - SARAO - Feb 2020
*/
#ifndef _TIME_H_
#define _TIME_H_

#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

void incr_microblaze_uptime_seconds(void);
u32 get_microblaze_uptime_seconds(void);

#ifdef __cplusplus
}
#endif
#endif
