#ifndef _CLI_H_
#define _CLI_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  /* sm states */
  CLI_INIT = 0,
  CLI_IDLE,
  CLI_BUILD_LINE,
  CLI_PARSE_CMD,
  CLI_PARSE_OPT,
  CLI_EXE,

  /* aux return states */
  CLI_IGNORE,
  CLI_EXIT
} CLI_STATE;

/* call to initialise the cli state machine */
CLI_STATE cli_init(void);

/* run the cli state machine */
CLI_STATE cli_sm(const char c);

#ifdef __cplusplus
}
#endif
#endif /*_CLI_H_*/
