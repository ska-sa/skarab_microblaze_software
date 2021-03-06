#include <xil_printf.h>
#include <xstatus.h>
#include <stdlib.h>

#include "cli.h"
#include "logging.h"
#include "register.h"
#include "constant_defs.h"
#include "i2c_master.h"
#include "delay.h"
#include "diagnostics.h"
#include "scratchpad.h"
#include "id.h"
#include "time.h"
#include "flash_sdram_controller.h"
#include "init.h"
#include "if.h"
#include "igmp.h"
#include "error.h"
#include "memtest.h"
#include "mezz.h"
#include "fanctrl.h"

#define LINE_BYTES_MAX 20

extern int _stack;
extern int _stack_end;
extern int _STACK_SIZE;

extern int _heap;
extern int _heap_end;
extern int _HEAP_SIZE;

/* internal state */
struct cli{
  char buffer[LINE_BYTES_MAX + 1];    /* extra one for '\0' */
  unsigned int index;
  char curr_char;
  unsigned int cmd_id;
  unsigned int curr_index;
  unsigned int opt_id;
  unsigned int opt_value_u32;
  CLI_STATE state;
  unsigned int run_again;    /* used to run the state machine again interneally before
                              * passing control back to calling function */
};

typedef enum {
  CMD_INDEX_LOG_LEVEL = 0,
  CMD_INDEX_LOG_SELECT,
  CMD_INDEX_BOUNCE_LINK,
  CMD_INDEX_TEST_TIMER,
  CMD_INDEX_GET_CONFIG,
  CMD_INDEX_GET_MEZZ_INV,
  CMD_INDEX_RESET_FW,
  CMD_INDEX_REBOOT_FPGA,
  CMD_INDEX_STATS,
  CMD_INDEX_WHOAMI,
  CMD_INDEX_UNAME,
  CMD_INDEX_UPTIME,
  CMD_INDEX_DUMP,
  CMD_INDEX_IF_MAP,
  CMD_INDEX_IGMP,
#ifndef PRUNE_CODEBASE_DIAGNOSTICS
  CMD_INDEX_PEEK,
#endif
  CMD_INDEX_WB_READ,
  CMD_INDEX_ARP_REQ,
  CMD_INDEX_ARP_PROC,
  CMD_INDEX_MEMTEST,
  CMD_INDEX_CLR_LINK_MON,
  CMD_INDEX_FAN_RUNTIME,
  CMD_INDEX_FAN_PWM_AVG,
  CMD_INDEX_HELP,
  CMD_INDEX_END
} CMD_INDEX;

typedef int (* cmd_callback)(struct cli *);

static const char * const cli_cmd_map[] = {
  [CMD_INDEX_LOG_LEVEL]   = "log-level",
  [CMD_INDEX_LOG_SELECT]  = "log-select",
  [CMD_INDEX_BOUNCE_LINK] = "bounce-link",
  [CMD_INDEX_TEST_TIMER]  = "test-timer",
  [CMD_INDEX_GET_CONFIG]  = "get-config",
  [CMD_INDEX_GET_MEZZ_INV]= "get-mezz-inv",
  [CMD_INDEX_RESET_FW]    = "reset-fw",
  [CMD_INDEX_REBOOT_FPGA] = "reboot-fpga",
  [CMD_INDEX_STATS]       = "stats",
  [CMD_INDEX_WHOAMI]      = "whoami",
  [CMD_INDEX_UNAME]       = "uname",
  [CMD_INDEX_UPTIME]      = "uptime",
  [CMD_INDEX_DUMP]        = "dump",
  [CMD_INDEX_IF_MAP]      = "if-map",
  [CMD_INDEX_IGMP]        = "igmp",
#ifndef PRUNE_CODEBASE_DIAGNOSTICS
  [CMD_INDEX_PEEK]        = "peek",
#endif
  [CMD_INDEX_WB_READ]     = "wb-read",
  [CMD_INDEX_ARP_REQ]     = "arp-req",
  [CMD_INDEX_ARP_PROC]    = "arp-proc",
  [CMD_INDEX_MEMTEST]     = "memtest",
  [CMD_INDEX_CLR_LINK_MON]= "clr-link-mon-count",
  [CMD_INDEX_FAN_RUNTIME] = "fan-runtime",
  [CMD_INDEX_FAN_PWM_AVG] = "fan-pwm-avg",
  [CMD_INDEX_HELP]        = "help",
  [CMD_INDEX_END]         = NULL
};

#define CLI_KEYWORD_HEX "HEX"
static const char * const cli_cmd_options[][12] = {
 [CMD_INDEX_LOG_LEVEL]    = {"trace",   "debug", "info", "warn", "error", "fatal",  "off",  NULL},
 [CMD_INDEX_LOG_SELECT]   = {"general", "dhcp",  "arp",  "icmp", "igmp",  "lldp",  "ctrl",   "buff", "hardw", "iface", "all", NULL},
 [CMD_INDEX_BOUNCE_LINK]  = {"0",       "1",     "2",    "3",    "4",     NULL},
 [CMD_INDEX_TEST_TIMER]   = { NULL },
 [CMD_INDEX_GET_CONFIG]   = { NULL },
 [CMD_INDEX_GET_MEZZ_INV] = { NULL },
 [CMD_INDEX_RESET_FW]     = { NULL },
 [CMD_INDEX_REBOOT_FPGA]  = { "",       "flash",  "sdram"},
 [CMD_INDEX_STATS]        = { NULL },
 [CMD_INDEX_WHOAMI]       = { NULL },
 [CMD_INDEX_UNAME]        = { NULL },
 [CMD_INDEX_UPTIME]       = { NULL },
 [CMD_INDEX_DUMP]         = {"stack",   "heap", /*, "text"*/ NULL},
 [CMD_INDEX_IF_MAP]       = { NULL },
 [CMD_INDEX_IGMP]         = { NULL },
#ifndef PRUNE_CODEBASE_DIAGNOSTICS
 [CMD_INDEX_PEEK]         = {"iface",   "dhcp"},
#endif
 [CMD_INDEX_WB_READ]      = {CLI_KEYWORD_HEX, NULL },
 [CMD_INDEX_ARP_REQ]      = {"off",     "on",     "stat"},    /* order of "off" (index 0) and "on" (index 1) are important */
 [CMD_INDEX_ARP_PROC]     = {"off",     "on",     "stat"},    /* order of "off" (index 0) and "on" (index 1) are important */
 [CMD_INDEX_MEMTEST]      = { NULL },
 [CMD_INDEX_CLR_LINK_MON] = { NULL },
 [CMD_INDEX_FAN_RUNTIME]  = {"lf",      "lm",     "lb",   "rb",   "fpga",   NULL },  /* order is important - corresponds to fan page number */
 [CMD_INDEX_FAN_PWM_AVG]  = {"lf",      "lm",     "lb",   "rb",   "fpga",   NULL },  /* order is important - corresponds to fan page number */
 [CMD_INDEX_HELP]         = { NULL },
 [CMD_INDEX_END]          = { NULL }
};

static int cli_log_level_exe(struct cli *_cli);
static int cli_log_select_exe(struct cli *_cli);
static int cli_bounce_link_exe(struct cli *_cli);
static int cli_test_timer_exe(struct cli *_cli);
static int cli_get_config_exe(struct cli *_cli);
static int cli_get_mezz_inv_exe(struct cli *_cli);
static int cli_reset_fw_exe(struct cli *_cli);
static int cli_reboot_fpga_exe(struct cli *_cli);
static int cli_stats_exe(struct cli *_cli);
static int cli_whoami_exe(struct cli *_cli);
static int cli_uname_exe(struct cli *_cli);
static int cli_uptime_exe(struct cli *_cli);
static int cli_dump_exe(struct cli *_cli);
static int cli_if_map_exe(struct cli *_cli);
static int cli_igmp_exe(struct cli *_cli);
#ifndef PRUNE_CODEBASE_DIAGNOSTICS
static int cli_peek_exe(struct cli *_cli);
#endif
static int cli_wb_read_exe(struct cli *_cli);
static int cli_arp_req_exe(struct cli *_cli);
static int cli_arp_proc_exe(struct cli *_cli);
static int cli_memtest_exe(struct cli *_cli);
static int cli_clr_link_mon_exe(struct cli *_cli);
static int cli_fan_runtime_exe(struct cli *_cli);
static int cli_fan_pwm_avg_exe(struct cli *_cli);
static int cli_help_exe(struct cli *_cli);

static const cmd_callback cli_cmd_callback[] = {
 [CMD_INDEX_LOG_LEVEL]    = cli_log_level_exe,
 [CMD_INDEX_LOG_SELECT]   = cli_log_select_exe,
 [CMD_INDEX_BOUNCE_LINK]  = cli_bounce_link_exe,
 [CMD_INDEX_TEST_TIMER]   = cli_test_timer_exe,
 [CMD_INDEX_GET_CONFIG]   = cli_get_config_exe,
 [CMD_INDEX_GET_MEZZ_INV] = cli_get_mezz_inv_exe,
 [CMD_INDEX_RESET_FW]     = cli_reset_fw_exe,
 [CMD_INDEX_REBOOT_FPGA]  = cli_reboot_fpga_exe,
 [CMD_INDEX_STATS]        = cli_stats_exe,
 [CMD_INDEX_WHOAMI]       = cli_whoami_exe,
 [CMD_INDEX_UNAME]        = cli_uname_exe,
 [CMD_INDEX_UPTIME]       = cli_uptime_exe,
 [CMD_INDEX_DUMP]         = cli_dump_exe,
 [CMD_INDEX_IF_MAP]       = cli_if_map_exe,
 [CMD_INDEX_IGMP]         = cli_igmp_exe,
#ifndef PRUNE_CODEBASE_DIAGNOSTICS
 [CMD_INDEX_PEEK]         = cli_peek_exe,
#endif
 [CMD_INDEX_WB_READ]      = cli_wb_read_exe,
 [CMD_INDEX_ARP_REQ]      = cli_arp_req_exe,
 [CMD_INDEX_ARP_PROC]     = cli_arp_proc_exe,
 [CMD_INDEX_MEMTEST]      = cli_memtest_exe,
 [CMD_INDEX_CLR_LINK_MON] = cli_clr_link_mon_exe,
 [CMD_INDEX_FAN_RUNTIME]  = cli_fan_runtime_exe,
 [CMD_INDEX_FAN_PWM_AVG]  = cli_fan_pwm_avg_exe,
 [CMD_INDEX_HELP]         = cli_help_exe,
 [CMD_INDEX_END]          = NULL
};


static void cli_reset_internals(struct cli *_cli);
static int cli_filter(char c);
static char ignore_case(char c);
static void cli_print_help(void);
static void cli_sm_run_again(struct cli *_cli);
static int cli_isspace(char c);
static int cli_strncmp(const char *first, const char *second, unsigned int n);
static int cli_valid_hex(char *string, unsigned int size);

typedef CLI_STATE (* const cli_state_func_ptr)(struct cli *);

static CLI_STATE cli_init_state(struct cli *);
static CLI_STATE cli_idle_state(struct cli *);
static CLI_STATE cli_build_line_state(struct cli *);
static CLI_STATE cli_parse_cmd_state(struct cli *);
static CLI_STATE cli_parse_opt_state(struct cli *);
static CLI_STATE cli_exe_state(struct cli *);

static cli_state_func_ptr cli_state_table[] = {
  [CLI_INIT]        = cli_init_state,
  [CLI_IDLE]        = cli_idle_state,
  [CLI_BUILD_LINE]  = cli_build_line_state,
  [CLI_PARSE_CMD]   = cli_parse_cmd_state,
  [CLI_PARSE_OPT]   = cli_parse_opt_state,
  [CLI_EXE]         = cli_exe_state
};


CLI_STATE cli_sm(const char c){
  static struct cli _cli = {.state = CLI_INIT, .run_again = 0};
  int ignore = 0;
  char c_lower;

  c_lower = ignore_case(c);

  ignore = cli_filter(c_lower);
  if (ignore){
    return CLI_IGNORE;
  }

  _cli.curr_char = c_lower;

  /* highest layer of control i.e. master key control */
  switch(c_lower){
    case 0x03:  /* CTRL-C */
      xil_printf("\r\n");
      cli_reset_internals(&_cli);
      _cli.state =  CLI_IDLE;
      return CLI_EXIT;
      break;

    default:
      break;
  }


  do {
    _cli.state = cli_state_table[_cli.state](&_cli);
    /* don't underflow - if zero, leave it zero */
    _cli.run_again = (_cli.run_again == 0) ? 0 : (_cli.run_again - 1);
  } while (_cli.run_again > 0);

  return _cli.state;
}

CLI_STATE cli_init(void){
  /*
   *  this function merely advances the state machine from the CLI_INIT state to the CLI_IDLE state. This is necessary
   *  for the cli state machine to immediately start parsing the characters input (instead of the one character delay in
   *  processing if this is not first called). The choice to wrap the cli_sm function with cli_init is due to the fact
   *  that the <struct cli> is private to the cli_sm function and would need to be made global (file scope) if a call to
   *  cli_reset_internals were to be made available via the API. The cli_sm function also needs a valid character to
   *  advance and hence the function is called with the '?' character.
   */

  return cli_sm('?');
}

static CLI_STATE cli_init_state(struct cli *_cli){

  cli_reset_internals(_cli);

  return CLI_IDLE;
}


static CLI_STATE cli_idle_state(struct cli *cli){
  /* wait here till we receive the '?' character
   * then go to build_line_state
   */

  /* first layer of character control */
  switch(cli->curr_char){
    case '?':
      cli_print_help();
      return CLI_BUILD_LINE;
      break;

    default:
      break;
  }

  return CLI_IDLE;
}



static CLI_STATE cli_build_line_state(struct cli *_cli){
  /* build line until one of three things happen:
   * 1) enter key hit, 2) buffer is full, 3) time-out
   */
  int i = _cli->index;
  int c;

#if 0
  /* input conditions which change state */
  /* NOTE: we can never overwrite the final position ('\0') */
  if (i >= (LINE_BYTES_MAX - 1)){
    xil_printf("\r\noverflow\r\n");
    cli_reset_internals(_cli);
    return CLI_IDLE;
  }
#endif

  switch (_cli->curr_char){
    case 0x0d:
      xil_printf("\r\n");
      cli_sm_run_again(_cli);   /* run the state machine again immediately in
                                   order to parse the line now */
      return CLI_PARSE_CMD;
      break;

    /* handle backspace / DEL */
    case 0x08:
    case 0x7f:
      if (i > 0){
        _cli->buffer[i-1] = '\0';
        _cli->index--;
      }
      break;

    default:
      /* NOTE: we can never overwrite the final position (always '\0') */
      if (i >= (LINE_BYTES_MAX - 1)){
        xil_printf("\r\noverflow\r\n");
        cli_reset_internals(_cli);
        return CLI_IDLE;
      }
      _cli->buffer[i] = _cli->curr_char;
      /* TODO: check that this cannot overrun array bounds */
      _cli->buffer[i + 1] = '\0';

      _cli->index++;
      break;
  }

#if 0
  /* handle backspace */
  if (_cli->curr_char == 0x7f){
    if (i > 0){
      _cli->buffer[i-1] = '\0';
      _cli->index--;
    }
  } else {

    _cli->buffer[i] = _cli->curr_char;
    /* TODO: check that this cannot overrun aray bounds */
    _cli->buffer[i + 1] = '\0';

    _cli->index++;
  }
  //_cli->buffer[_cli->index] = '\0';
#endif

  /* clear line */
  xil_printf("\r");
  for (c = 0; c < LINE_BYTES_MAX; c++){
    xil_printf("%c", ' ');
  }
  /* print line */
  xil_printf("\r%s", _cli->buffer);

  return CLI_BUILD_LINE;
}

static CLI_STATE cli_parse_cmd_state(struct cli *_cli){
  volatile int i = 0;
  int j = 0;
  int m = 0;
  char tmp_buffer[LINE_BYTES_MAX + 1] = {'\0'};

  /* skip over any initial whitespaces */
  while ((cli_isspace(_cli->buffer[i]) == 1) && (i <= (LINE_BYTES_MAX - 1))){
    i++;
  }

  /* an empty buffer or filled with whitespace */
  if ((i > (LINE_BYTES_MAX - 1)) || (_cli->buffer[i] == '\0')){
    xil_printf("{none}\r\n");
    cli_reset_internals(_cli);
    return CLI_IDLE;
  }

  /* get the cmd */
  for (i; i <= (_cli->index); i++){
    /* space or terminating null character - stop */
    if ((cli_isspace(_cli->buffer[i]) == 1) || (_cli->buffer[i] == '\0')){
      break;
    }
    tmp_buffer[j] = _cli->buffer[i];
    j++;
  }
  tmp_buffer[j] = '\0';

  while(cli_cmd_map[m] != NULL){
    if (cli_strncmp(tmp_buffer, cli_cmd_map[m], j+1)){
      _cli->cmd_id = m;
      _cli->curr_index = i; 
      cli_sm_run_again(_cli);
      return CLI_PARSE_OPT;
    }
    m++;
  }

  xil_printf("%s {invalid cmd}\r\n", tmp_buffer);
  cli_reset_internals(_cli);

  return CLI_IDLE;
}

static CLI_STATE cli_parse_opt_state(struct cli *_cli){
  volatile int i;
  int j = 0;
  int m = 0;
  char tmp_buffer[LINE_BYTES_MAX + 1] = {'\0'};

  i = _cli->curr_index;

  /* skip over any initial whitespaces */
  while ((cli_isspace(_cli->buffer[i]) == 1) && (i <= LINE_BYTES_MAX - 1)){
    i++;
  }

  /* get the option */
  for (i; i <= (_cli->index); i++){
    /* space or terminating null character - stop */
    if ((cli_isspace(_cli->buffer[i]) == 1) || (_cli->buffer[i] == '\0')){
      break;
    }
    tmp_buffer[j] = _cli->buffer[i];
    j++;
  }
  tmp_buffer[j] = '\0';

  /* first check for cli keywords... */
  if (cli_strncmp(CLI_KEYWORD_HEX, cli_cmd_options[_cli->cmd_id][m], 4)){
    /* check that the argument is a valid *hex* format */
    /* and only provide support for 32-bit hex values: 0xffffffff */
    if (cli_valid_hex(tmp_buffer, j+1) && ((j+1) <= 11)){
      /* convert hex string to a value */
      _cli->opt_value_u32 = (unsigned int) strtoul(tmp_buffer, NULL, 16);    /* the string has already been
                                                                             * validated, therefore pass in NULL as
                                                                             * 2nd arg */
      _cli->opt_id = m;
      _cli->curr_index = i;
      return CLI_EXE;
    } else {
      xil_printf("%s %s%s\r\n",cli_cmd_map[_cli->cmd_id], tmp_buffer,\
          /* space formatting -> */ tmp_buffer[0] != '\0' ? " {invalid hex value, expected 32-bit format 0x1234abcd}" : "{hex value required}");

      cli_reset_internals(_cli);
      return CLI_IDLE;
    }
  }

  /* if not a keyword then parse the arguments... */
  while(cli_cmd_options[_cli->cmd_id][m] != NULL){
    if (cli_strncmp(tmp_buffer, cli_cmd_options[_cli->cmd_id][m], j+1)){
      _cli->opt_id = m;
      _cli->curr_index = i;
      return CLI_EXE;
    }
    m++;
  }

  /* can't this be collapsed into the above while-loop? */
  if (0 == m){   /* i.e. cli_cmd_options[_cli->cmd_id][0] == NULL*/
    /* no option argument for this cmd - so execute callback as well */
    _cli->opt_id = m;
    _cli->curr_index = i;
    return CLI_EXE;
  }

  xil_printf("%s %s%s\r\n",cli_cmd_map[_cli->cmd_id], tmp_buffer,\
      /* space formatting -> */ tmp_buffer[0] != '\0' ? " {invalid option}" : "{option required}");

  cli_reset_internals(_cli);

  return CLI_IDLE;
}

static CLI_STATE cli_exe_state(struct cli *_cli){
  int ret = 1;

  /* run the relevant cmd callback according to the index */
  if (cli_cmd_callback[_cli->cmd_id] != NULL){
    ret = cli_cmd_callback[_cli->cmd_id](_cli);
  }

  xil_printf("%s %s",cli_cmd_map[_cli->cmd_id],\
      cli_cmd_options[_cli->cmd_id][_cli->opt_id] == NULL ? "" : cli_cmd_options[_cli->cmd_id][_cli->opt_id]);

  if (ret == 0){
    xil_printf(" {ok}\r\n");
  } else {
    xil_printf(" {fail}\r\n");
  }
  cli_reset_internals(_cli);
  return CLI_IDLE;
}

static void cli_reset_internals(struct cli *_cli){
  int i;

  for (i = 0; i < (LINE_BYTES_MAX + 1); i++){
    _cli->buffer[i] = '\0';
  }
  _cli->index = 0;
  _cli->curr_char = '\0';
  _cli->run_again = 0;
  _cli->cmd_id = 0;
  _cli->curr_index = 0; 
  _cli->opt_id = 0;
}

static void cli_sm_run_again(struct cli *_cli){
  _cli->run_again += 2;
}

static char ignore_case(char c){
  if((c >= 'A') && (c <= 'Z')){
    return (c + 0x20);
  } else {
    return c;
  }
}


static int cli_filter(char c){

  /* pass through basic keyboard key ranges */
  /* numbers */
  if ((c >= 0x30) &&
      (c <= 0x39))
  {
    return 0;
  }

  /* letters */
  if ((c >= 0x61) &&
      (c <= 0x7A))
  {
    return 0;
  }


  /* pass through some special characters - CR, space, dot, ... */
  switch (c){
    case '\0':      /* on blocking calls of this state machine, we need to
                       interpret the initial value of the input character, i.e.
                       NULL, in order to advance the SM out of reset on first
                       invocation. */
      /* fall-thru */
    case '.':
      /* fall-thru */
    case '?':
      /* fall-thru */
    case ' ':
      /* fall-thru */
    case '-':
      /* fall-thru */
    case 0x3:       /* CTRL-C */
      /* fall-thru */
    case 0x0d:      /* CR */
      /* fall-thru */
    case 0x08:      /* DEL */
    case 0x7f:      /* BACKSPACE */
      return 0;
      break;

    default:
      break;
  }

  /* ignore everything else */
  return 1;
}

/* FIXME: these string functions probably need work */

/* return 1 if space, 0 if not */
static int cli_isspace(char c){

  switch(c){
    case ' ':
      /* fall-thru */
    case '\f':
      /* fall-thru */
    case '\n':
      /* fall-thru */
    case '\r':
      /* fall-thru */
    case '\t':
      /* fall-thru */
    case '\v':
      return 1;
      break;

    default:
      break;
  }

  return 0;
}

/* return 1 if equal, 0 if not */
static int cli_strncmp(const char *first, const char *second, unsigned int n){
  unsigned int i;

  for (i = 0; i < n; i++){
    if (first[i] != second[i]){
      return 0;
    }
    if ((first[i] == '\0') && (second[i] == '\0')){
      return 1;
    }
  }

  return 0;
}

/* return 1 if hex, return 0 if not */
/* valid hex format: 0x0000000... */
static int cli_valid_hex(char *string, unsigned int size){
  unsigned int i;

  /*
   * must have at least one digit, therefore
   * size must be at least 4 => '0x' plus 1 digit plus '\0'
   */
  if (size < 4){
    return 0;
  }

  /* check '0x' prefix */
  if ((string[0] != '0') || (string[1] != 'x')){
    return 0;
  }

  /* the characters in the cli are always converted to lower case */
  /* check validity of each digit */
  for (i = 2; i < (size-1); i++){

    /* number or hex letter? */
    if (((string[i] >= 0x30) && (string[i] <= 0x39)) || ((string[i] >= 0x61) && (string[i] <= 0x66))) {
      continue; /* valid => check next char by advancing for-loop*/
    } else {
      return 0; /* not a valid char */
    }
  }

  /* if we reach here we have a valid hex format */
  return 1;
}


static void cli_print_help(void){
  int i = 0;
  int j = 0;

  xil_printf("usage:\r\n");
  i = 0;
  while(cli_cmd_map[i] != NULL){
    xil_printf("%s%s", cli_cmd_map[i], cli_cmd_options[i][0] == NULL ? "\r\n" : " [");
    j = 0;
    while(cli_cmd_options[i][j] != NULL){
      xil_printf("%s", cli_cmd_options[i][j]);
      /* determine if this is the last of the options */
      cli_cmd_options[i][j+1] == NULL ? xil_printf("]\r\n") : xil_printf("|");
      j++;
    }
    i++;
  }
  /*xil_printf("log-level [trace|debug|info|warn|error|off]\r\n");*/
  /*xil_printf("log-select [general|dhcp|arp|icmp|lldp|ctrl|buff|hardw|iface|all]\r\n");*/
}


static int cli_log_select_exe(struct cli *_cli){
  u8 cache_log_select = 0;
  /* TODO: some error checking perhaps? */
  xil_printf("running log-select %d\r\n", _cli->opt_id);


  /* cache the log select in persistent memory
   * bit8 => set when this cmd issued to indicate a manual change in log-select upon reset
   * bit7-0 => cached log select
   */
  cache_log_select = (_cli->opt_id) | 0x80;

  xil_printf("caching log-select value 0x%02x to pmem\r\n", cache_log_select);
  PersistentMemory_WriteByte(LOG_SELECT_STARTUP_INDEX, cache_log_select);

  set_log_select(_cli->opt_id);
  return 0;
}

static int cli_log_level_exe(struct cli *_cli){
  u8 cache_log_level = 0;
  /* TODO: some error checking perhaps? */
  xil_printf("running log-level %d\r\n", _cli->opt_id);

  /* cache the log level in persistent memory
   * bit8 => set when this cmd issued to indicate a manual change in log-level upon reset
   * bit7-0 => cached log level
   */
  cache_log_level = (_cli->opt_id) | 0x80;

  xil_printf("caching log-level value 0x%02x to pmem\r\n", cache_log_level);
  PersistentMemory_WriteByte(LOG_LEVEL_STARTUP_INDEX, cache_log_level);

  set_log_level(_cli->opt_id);
  return 0;
}

static int cli_bounce_link_exe(struct cli *_cli){
  u32 l;

  /* check if the link is present - the option parsing state will catch the other end of the error modes */
  if (IF_ID_PRESENT != check_interface_valid(_cli->opt_id)){
    xil_printf("link %d not present\r\n", _cli->opt_id);
    return -1;
  }

  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_DEBUG, "cli opt id: %d\r\n", _cli->opt_id);

  if (_cli->opt_id == 0){
    /* 1gbe - always link 0 */
    UpdateGBEPHYConfiguration();
  } else {
    /* 40gbe link */
    l = 0x1 << _cli->opt_id;

    /* FIXME: remove global scope of uQSFPCtrlReg */
    uQSFPCtrlReg = uQSFPCtrlReg | l;
    WriteBoardRegister(C_WR_ETH_IF_CTL_ADDR, uQSFPCtrlReg);
  }

  return 0;
}

/* This function simply prints out "0..1..2..3..4..5" with a one second delay between integers.
 * It is used to verify that the timer infrastructure is built correctly and thus gives us
 * the one second interval reference count as a sanity check
 */
static int cli_test_timer_exe(struct cli *_cli){
#if 0
  unsigned int i;
#define COUNT 5
  for (i = 0; i <= COUNT; i++){
    log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "%d%s", i, i < COUNT ? ".." : "\r\n");
    Delay(1000000);
  }
#endif
  test_timer_enable();
  return 0;
}


static int cli_get_config_exe(struct cli *_cli){

#ifdef DO_SANITY_CHECKS
  const char *str_sanity = "yes";
#else
  const char *str_sanity = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "do sanity checks: %s\r\n", str_sanity);

#ifdef REDUCED_CLK_ARCH
  const char *str_clk = "yes";
#else
  const char *str_clk = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "reduced clk: %s\r\n", str_clk);

#ifdef WISHBONE_LEGACY_MAP
  const char *str_wb = "yes";
#else
  const char *str_wb = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "wishb legacy map: %s\r\n", str_wb);

#ifdef MULTILINK_ARCH
  const char *str_multi = "yes";
#else
  const char *str_multi = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "multi-link arch: %s\r\n", str_multi);

#ifdef LINK_MONITOR
  const char *str_dmon = "yes";
#else
  const char *str_dmon = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "link mon: %s\r\n", str_dmon);

#ifdef HMC_RECONFIG_RETRY
  const char *str_hmc = "yes";
#else
  const char *str_hmc = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "hmc retry: %s\r\n", str_hmc);

#ifdef PREEMPT_CONFIGURE_FABRIC_IF
  const char *str_pre_ip = "yes";
#else
  const char *str_pre_ip = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "pre-config 40gbe link 1: %s\r\n", str_pre_ip);

#ifdef PRUNE_CODEBASE_DIAGNOSTICS
  const char *str_prune = "yes";
#else
  const char *str_prune = "no";
#endif
  log_printf(LOG_SELECT_GENERAL, LOG_LEVEL_INFO, "prune code: %s\r\n", str_prune);

  return 0;
}


static int cli_reset_fw_exe(struct cli *_cli){
  log_printf(LOG_SELECT_DHCP, LOG_LEVEL_WARN, "Resetting firmware...\r\n");

  /* just wait a little while to enable serial port to finish writing out */
  Delay(100000); /* 100ms */

  WriteBoardRegister(C_WR_BRD_CTL_STAT_0_ADDR, 1 << 30);    /* set bit 30 of board ctl reg 0 to reset the fw */

  return 0;
}


static int cli_stats_exe(struct cli *_cli){
  u8 n, link, phy_id;
  struct sIFObject *iface;

  n = get_num_interfaces();

  for (link = 0; link < n; link++){
    phy_id = get_physical_interface_id(link);
    iface = lookup_if_handle_by_id(phy_id);
    PrintInterfaceCounters(iface);
  }

  return 0;
}

static int cli_whoami_exe(struct cli *_cli){
  u8 uSerial[ID_SK_SERIAL_LEN];
  int status;

  status = get_skarab_serial(uSerial, ID_SK_SERIAL_LEN);
  if (status != XST_SUCCESS){
    xil_printf("failed\r\n");
    return -1;
  }

  xil_printf("skarab%02x%02x%02x\r\n", uSerial[1], uSerial[2], uSerial[3]);

  return 0;
}

static int cli_uname_exe(struct cli *_cli){
  char *v = (char *) GetVersionInfo();

  xil_printf("%s ", v);

  if (((ReadBoardRegister(C_RD_VERSION_ADDR) & 0xff000000) >> 24) == 0){
    xil_printf("toolflow");
  } else {
    xil_printf("bsp");
  }

  xil_printf("\r\n");

  return 0;
}

static int cli_uptime_exe(struct cli *_cli){
  u32 uptime;

  uptime = get_microblaze_uptime_seconds();

  xil_printf("%d seconds\r\n", uptime);

  return 0;
}

static int cli_help_exe(struct cli *_cli){
  cli_print_help();
  return 0;
}

#define STACK 0
#define HEAP  1
#define TEXT  2

static void aux_cli_print_mem_region_bytes(const u8 * const start_addr, const unsigned int size){
  u8 *ptr = NULL;
  int i=0;

  for (ptr = (u8 *) start_addr; ptr < ((u8 *) start_addr + size ); ptr++){

#if 0
    eol = ((i % 4) == 0) ? "\r\n" : " ";
    xil_printf("%p %02x%s", ptr, *ptr, eol);
#else
    if ((i == 0) || ((i % 4) == 0)){
      xil_printf("\r\n0x%p ", ptr);
    } else {
      xil_printf(" ");
    }
    xil_printf("%02x", *ptr);
#endif

#if 0
    if ((i % 4) == 0) {
      xil_printf("\r\n");
    }
#endif
    i++;
  }

  xil_printf("\r\n");
}


static int cli_dump_exe(struct cli *_cli){

  /* WARNING: this function dumps a block of memory to the serial console,
   * which is a relatively slow operation. The block size needs to be kept
   * relatively compact since this loop will not kick the wdt. (TODO FIXME)
   */

  /* the stack grows upward so the end of the stack is at a lower mem
   * address and the start of the stack is at a higher mem location
   */

  int *mem_start;
  //int *mem_end;
  unsigned int size;
  //char *eol;

  switch(_cli->opt_id){
    case STACK:
      mem_start = (int *) &_stack_end;
      //mem_end = (int *) &_stack;
      size = (unsigned int) &_STACK_SIZE;

      //xil_printf("@%p @%p %ub\r\n", mem_start, mem_end, size);

      aux_cli_print_mem_region_bytes((u8 *) mem_start, size);

      break;

    case HEAP:
      aux_cli_print_mem_region_bytes((u8 *) &_heap, (unsigned int) &_HEAP_SIZE);
      break;

#if 0
    case TEXT:
      /* TODO */
      break;
#endif

    default:
      break;
  }

  return 0;
}

#define CLI_RBT_NOARG 0
#define CLI_RBT_FLASH 1
#define CLI_RBT_SDRAM 2

static int cli_reboot_fpga_exe(struct cli *_cli){
  u8 aux_flags = 0;
  PersistentMemory_ReadByte(AUX_SKARAB_FLAGS_INDEX, &aux_flags);
  xil_printf("aux_flags = %d\r\n", aux_flags);

  switch(_cli->opt_id){
    case CLI_RBT_NOARG:
      xil_printf("rebooting from previous location\r\n");
      sudo_reboot_now_from_last_location();
      break;

    case CLI_RBT_FLASH:
      xil_printf("rebooting from flash\r\n");
      sudo_reboot_now_from_flash_location();
      break;

    case CLI_RBT_SDRAM:
      xil_printf("rebooting from sdram\r\n");
      sudo_reboot_now_from_sdram_location();
      break;

    default:
      break;
  }

  return 0;
}


static int cli_if_map_exe(struct cli *_cli){
  print_interface_map();
  return 0;
}


static int cli_igmp_exe(struct cli *_cli){
  vIGMPPrintInfo();
  return 0;
}

#ifndef PRUNE_CODEBASE_DIAGNOSTICS
#define CLI_PEEK_IFACE  0
#define CLI_PEEK_DHCP   1

static int cli_peek_exe(struct cli *_cli){
  u8 logical_link;
  u8 physical_interface_id;
  u8 n;

  n = get_num_interfaces();

  /* TODO: could optimise this code a bit more */
  switch(_cli->opt_id){
    case CLI_PEEK_IFACE:
      for (logical_link = 0; logical_link < n; logical_link++){
        physical_interface_id = get_physical_interface_id(logical_link);
        print_interface_internals(physical_interface_id);
      }
      break;

    case CLI_PEEK_DHCP:
      for (logical_link = 0; logical_link < n; logical_link++){
        physical_interface_id = get_physical_interface_id(logical_link);
        print_dhcp_internals(physical_interface_id);
      }
      break;

    default:
      break;
  }

  return 0;
}
#endif


static int cli_wb_read_exe(struct cli *_cli){
  u32 addr;
  u32 data;
  u8 errno;

  addr = _cli->opt_value_u32;
  data = ReadWishboneRegister(addr);

  /* check for wb memory addressing errors (out-of-range address)*/
  errno = read_and_clear_error_flag();

  if (errno == ERROR_AXI_DATA_BUS){
    xil_printf("bus error\r\n") ;
    return -1;
  } else {
    xil_printf("addr 0x%08x, data 0x%08x\r\n", addr, data) ;
    return 0;
  }
}


#define CLI_ARP_STAT   2
static int cli_arp_req_exe(struct cli *_cli){
  u8 logical_link;
  u8 physical_interface_id;
  u8 n, stat;
  struct sIFObject *iface;

  n = get_num_interfaces();
  if (n < 1){
    return -1;
  }

  if (CLI_ARP_STAT == _cli->opt_id){
    /* show current status - all interfaces *should* have the same setting since this function loops through them when
     * setting it so read only the first one */
    physical_interface_id = get_physical_interface_id(0);
    iface = lookup_if_handle_by_id(physical_interface_id);
    stat = iface->uIFEnableArpRequests;   /* 0 or 1 */
    /* display status */
    xil_printf("%s %s\r\n", cli_cmd_map[_cli->cmd_id], cli_cmd_options[_cli->cmd_id][stat]);
  } else {
    /* TODO: create API function to enable/disable arp req on interface by id */
    for (logical_link = 0; logical_link < n; logical_link++){
      physical_interface_id = get_physical_interface_id(logical_link);
      iface = lookup_if_handle_by_id(physical_interface_id);
      iface->uIFEnableArpRequests = _cli->opt_id;   /* ensure the option id maps to arp req enable/disable */
    }
  }

  return 0;
}


static int cli_arp_proc_exe(struct cli *_cli){
  u8 logical_link;
  u8 physical_interface_id;
  u8 n, stat;
  struct sIFObject *iface;

  n = get_num_interfaces();
  if (n < 1){
    return -1;
  }

  if (CLI_ARP_STAT == _cli->opt_id){
    /* show current status - all interfaces *should* have the same setting since this function loops through them when
     * setting it so read only the first one */
    physical_interface_id = get_physical_interface_id(0);
    iface = lookup_if_handle_by_id(physical_interface_id);
    stat = iface->uIFEnableArpProcessing;   /* 0 or 1 */
    /* display status */
    xil_printf("%s %s\r\n", cli_cmd_map[_cli->cmd_id], cli_cmd_options[_cli->cmd_id][stat]);
  } else {
    /* TODO: create API function to enable/disable arp proc on interface by id */
    for (logical_link = 0; logical_link < n; logical_link++){
      physical_interface_id = get_physical_interface_id(logical_link);
      iface = lookup_if_handle_by_id(physical_interface_id);
      iface->uIFEnableArpProcessing = _cli->opt_id;   /* ensure the option id maps to arp proc enable/disable */
    }
  }

  return 0;
}


static int cli_memtest_exe(struct cli *_cli){

  vRunMemoryTest();

  return 0;
}



static int cli_get_mezz_inv_exe(struct cli *_cli){
  u8 mezz;

  for (mezz = 0; mezz < 4; mezz++){
    /* these functions print mezz type, for hw and fw, internally */
    read_mezz_type_id(mezz);
    get_mezz_firmware_type(mezz);
  }

  return 0;
}


static int cli_clr_link_mon_exe(struct cli *_cli){
  u8 PMemState = PMEM_RETURN_ERROR;
  tPMemReturn ret;

  PMemState = PersistentMemory_Check();

  if (PMemState != PMEM_RETURN_ERROR){
    ret = PersistentMemory_WriteByte(DHCP_RECONFIG_COUNT_INDEX, 0);

    if (PMEM_RETURN_OK == ret){
      return 0;
    } else {
      return -1;
    }
  } else {
    return -1;
  }
}

const char * const fan_text_lut[] = {"left-front", "left-mid", "left-back", "right-back", "fpga"};

static int cli_fan_runtime_exe(struct cli *_cli){
  u16 runtime;

  /* no need to check the option parsed in - cli s/m logic does this already, we should
   * thus be able to pass it directly to the fan control runtime function.
   */
  runtime = fanctrlr_get_run_time(_cli->opt_id);
  if (0 == runtime){
    return -1;
  }

#ifndef PRUNE_CODEBASE_DIAGNOSTICS
  xil_printf("%s ", fan_text_lut[_cli->opt_id]);
#endif
  xil_printf("%u hrs\r\n", runtime);

  return 0;
}


static int cli_fan_pwm_avg_exe(struct cli *_cli){
  u16 pwm_avg;

  /* no need to check the option parsed in - cli s/m logic does this already, we should
   * thus be able to pass it directly to the fan control pwm avg function.
   */
  pwm_avg = fanctrlr_get_pwm_avg(_cli->opt_id);
  if (0 == pwm_avg){
    return -1;
  }

#ifndef PRUNE_CODEBASE_DIAGNOSTICS
  xil_printf("%s ", fan_text_lut[_cli->opt_id]);
#endif
  xil_printf("%u.%u %%\r\n", (pwm_avg/100), (pwm_avg%100));

  return 0;
}
