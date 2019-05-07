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

/* LOG_LEVEL_ALWAYS has highest priority overrides any other filter (i.e. log-level or log-select) */
#define log_printf(select, level, ...) if ((get_log_select() == LOG_SELECT_ALL) || \
                                          ((select) == get_log_select()) ||\
                                          ((level) == LOG_LEVEL_ALWAYS)){\
                                            if ((((level) >= get_log_level()) && \
                                               ((level) < LOG_LEVEL_OFF)) || \
                                               ((level) == LOG_LEVEL_ALWAYS)) \
                                                  {xil_printf(__VA_ARGS__);}}

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

typedef enum {
LOG_SELECT_GENERAL = 0,
LOG_SELECT_DHCP,
LOG_SELECT_ARP,
LOG_SELECT_ICMP,
LOG_SELECT_LLDP,
LOG_SELECT_CTRL,
LOG_SELECT_BUFF,
LOG_SELECT_HARDW,
LOG_SELECT_IFACE,
LOG_SELECT_ALL,
LOG_SELECT_MAX
} tLogSelect;

#ifdef __cplusplus
extern "C" {
#endif

/* return pointer to string of log level name */
const char *get_level_string(tLogLevel l);

void set_log_level(tLogLevel l);
tLogLevel get_log_level(void);

void cache_log_level(void);
void restore_log_level(void);

const char *get_select_string(tLogSelect s);

tLogSelect get_log_select(void);
void set_log_select(tLogSelect s);

#ifdef __cplusplus
}
#endif

#endif /* _LOGGING_H_ */
