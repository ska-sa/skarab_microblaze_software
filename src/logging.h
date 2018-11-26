#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <xil_printf.h>

/*
 * hand was slightly forced to implement it this way since there is no variadic
 * version of the xil_printf function in the xil lib. Not sure if this is the
 * best and most optimal solution since we're injecting conditionals everywhere!
 *
 *  TODO's
 *  - store and read log level from motherboard eeprom
 *  - allow for a compile time pruning mechanism
 */

#define log_printf(level, ...) if ((((level) >= get_log_level()) && \
                                    ((level) < LOG_LEVEL_OFF)) || \
                                    ((level) == LOG_LEVEL_ALWAYS)) \
                                      {xil_printf(__VA_ARGS__);}

typedef enum {
LOG_LEVEL_TRACE = 0,
LOG_LEVEL_DEBUG,
LOG_LEVEL_INFO,
LOG_LEVEL_WARN,
LOG_LEVEL_ERROR,
LOG_LEVEL_FATAL,
LOG_LEVEL_OFF,
LOG_LEVEL_ALWAYS,
LOG_LEVEL_MAX
} tLogLevel;


#ifdef __cplusplus
extern "C" {
#endif

/* return pointer to string of log level name */
const char *get_level_string(tLogLevel l);

void set_log_level(tLogLevel l);
tLogLevel get_log_level(void);

void cache_log_level(void);
void restore_log_level(void);

#ifdef __cplusplus
}
#endif

#endif /* _LOGGING_H_ */
