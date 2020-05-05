#include "error.h"

static volatile u8 error_flag = 0;

void set_error_flag(u8 errno){
  error_flag = errno;
}

u8 read_and_clear_error_flag(void){
  u8 temp_flag = error_flag;
  error_flag = 0;
  return temp_flag;
}
