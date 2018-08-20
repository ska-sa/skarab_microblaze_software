#ifndef _ID_H_
#define _ID_H_
#include <xil_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID_SK_SERIAL_LEN 4
#define ID_PX_SERIAL_LEN 3

u8 get_skarab_serial(u8 *sk_buffer, u8 sk_length);
u8 get_peralex_serial(u8 *px_buffer, u8 px_length);

#ifdef __cplusplus
}
#endif

#endif
