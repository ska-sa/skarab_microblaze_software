#include <xstatus.h>
#include <xil_assert.h>

#include "logging.h"

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

/* log level - default set to "debug" level */
static volatile tLogLevel trace_level = LOG_LEVEL_DEBUG;
static volatile tLogLevel cached_level = LOG_LEVEL_DEBUG;

const char *get_level_string(tLogLevel l){
  /* Assert correct API usage */
  Xil_AssertNonvoid((l >= 0) && (l < LOG_LEVEL_MAX));

  return level_str[l];
}

void set_log_level(tLogLevel l){
  /* error checking */
  if ((l < 0) || (l >= LOG_LEVEL_MAX)){
    log_printf(LOG_LEVEL_ERROR, "Log-level out of range - falling back to \"DEBUG\" level\r\n");
    l = LOG_LEVEL_DEBUG;
  }
  trace_level = l;
  log_printf(LOG_LEVEL_ALWAYS, "Setting log-level to: %s\r\n", get_level_string(l));
}

tLogLevel get_log_level(void){
  return trace_level;
}

void cache_log_level(void){
  cached_level = trace_level;
}

void restore_log_level(void){
  trace_level = cached_level;
}
