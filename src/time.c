/*
   rvw
   SARAO
   Feb 2020
*/

#include <xil_types.h>
#include "time.h"

struct skarab_time{
  u32 microblaze_uptime_seconds;
  /* u32 skarab_uptime_seconds; */
};

static struct skarab_time *get_time_handle(void);

void incr_microblaze_uptime_seconds(void){
  struct skarab_time *t;

  t = get_time_handle();

  t->microblaze_uptime_seconds++;
}

u32 get_microblaze_uptime_seconds(void){
  struct skarab_time *t;

  t = get_time_handle();

  return t->microblaze_uptime_seconds;
}

/* wrapper for time struct */
static struct skarab_time *get_time_handle(void){
  static struct skarab_time t = {0};

  return &t;
}
