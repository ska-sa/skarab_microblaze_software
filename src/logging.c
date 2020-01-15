#include <xstatus.h>
#include <xil_assert.h>

#include "logging.h"

/* log-level string lookup table */
static const char *level_str[LOG_LEVEL_MAX] = {
  [LOG_LEVEL_TRACE]   = "trace",
  [LOG_LEVEL_DEBUG]   = "debug",
  [LOG_LEVEL_INFO]    = "info",
  [LOG_LEVEL_WARN]    = "warn",
  [LOG_LEVEL_ERROR]   = "error",
  [LOG_LEVEL_FATAL]   = "fatal",
  [LOG_LEVEL_OFF]     = "off",
  [LOG_LEVEL_ALWAYS]  = "always"
};

/* log-select string lookup table */
static const char *select_str[LOG_SELECT_MAX] = {
  [LOG_SELECT_GENERAL] = "general",
  [LOG_SELECT_DHCP]    = "dhcp",
  [LOG_SELECT_ARP]     = "arp",
  [LOG_SELECT_ICMP]    = "icmp",
  [LOG_SELECT_IGMP]    = "igmp",
  [LOG_SELECT_LLDP]    = "lldp",
  [LOG_SELECT_CTRL]    = "control",
  [LOG_SELECT_BUFF]    = "buffer",
  [LOG_SELECT_HARDW]   = "hardware",
  [LOG_SELECT_IFACE]   = "interface",
  [LOG_SELECT_ALL]     = "all"
};

/* log level - default set to "debug" level */
static volatile tLogLevel current_level = LOG_LEVEL_DEBUG;
static volatile tLogLevel cached_level = LOG_LEVEL_DEBUG;

/* finer grained logging - which sub-feature to log - default is "all" */
static volatile tLogSelect current_select = LOG_SELECT_ALL;


/*********** log-level ***********/
const char *get_level_string(tLogLevel l){
  /* Assert correct API usage */
  Xil_AssertNonvoid((l >= 0) && (l < LOG_LEVEL_MAX));

  return level_str[l];
}

void set_log_level(tLogLevel l){
  /* error checking */
  if ((l < 0) || (l >= LOG_LEVEL_MAX)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Log-level out of range - falling back to \"DEBUG\" level\r\n");
    l = LOG_LEVEL_DEBUG;
  }
  current_level = l;
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "Setting log-level to: %s\r\n", get_level_string(l));
}

tLogLevel get_log_level(void){
  return current_level;
}

void cache_log_level(void){
  cached_level = current_level;
}

void restore_log_level(void){
  current_level = cached_level;
}


/*********** log-select ***********/
const char *get_select_string(tLogSelect s){
  /* Assert correct API usage */
  Xil_AssertNonvoid((s >= 0) && (s < LOG_SELECT_MAX));

  return select_str[s];
}

tLogSelect get_log_select(void){
  return current_select;;
}

void set_log_select(tLogSelect s){
  /* error checking */
  if ((s < 0) || (s >= LOG_SELECT_MAX)){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ERROR, "Log-select out of range - falling back to \"ALL\" mode\r\n");
    s = LOG_SELECT_ALL;
  }
  current_select = s;
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_ALWAYS, "Setting log-select to: %s\r\n", get_select_string(s));
}
