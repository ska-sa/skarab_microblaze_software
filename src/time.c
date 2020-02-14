/*
   rvw
   SARAO
   Feb 2020
*/

#include <xil_types.h>
#include <xil_printf.h>
#include "time.h"

struct skarab_time{
  u32 microblaze_uptime_seconds;
  /* u32 skarab_uptime_seconds; */
  int test_timer_count;
};

static struct skarab_time *get_time_handle(void);

void incr_microblaze_uptime_seconds(void){
  struct skarab_time *t;

  t = get_time_handle();

  t->microblaze_uptime_seconds++;

  /* this is used to run a test-timer as part of the cli cmd */
  if (t->test_timer_count){
    xil_printf("%ds\r\n", t->test_timer_count);
    t->test_timer_count--;
  }
}

u32 get_microblaze_uptime_seconds(void){
  struct skarab_time *t;

  t = get_time_handle();

  return t->microblaze_uptime_seconds;
}

void test_timer_enable(void){
  struct skarab_time *t;

  t = get_time_handle();

  t->test_timer_count = 5;
}

/* wrapper for time struct */
static struct skarab_time *get_time_handle(void){
  static struct skarab_time t = {0,0};

  return &t;
}
