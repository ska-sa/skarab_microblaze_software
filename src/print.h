#ifndef _PRINT_H_
#define _PRINT_H_

#include <xil_printf.h>

#include "constant_defs.h"

/* enable all levels above selected print level */
#ifdef TRACE_PRINT
#define DEBUG_PRINT
#define INFO_PRINT
#define WARN_PRINT
#define ERROR_PRINT
#endif

#ifdef DEBUG_PRINT
#define INFO_PRINT
#define WARN_PRINT
#define ERROR_PRINT
#endif

#ifdef INFO_PRINT
#define WARN_PRINT
#define ERROR_PRINT
#endif

#ifdef WARN_PRINT
#define ERROR_PRINT
#endif

/* WARNING: following variadic macro only allowed in C99 and later*/
#define always_printf(...) xil_printf(__VA_ARGS__)

#ifdef ERROR_PRINT
  #define error_printf(...) xil_printf(__VA_ARGS__)
#else
  #define error_printf(...)
#endif

#ifdef WARN_PRINT
  #define warn_printf(...) xil_printf(__VA_ARGS__)
#else
  #define warn_printf(...)
#endif

#ifdef INFO_PRINT
  #define info_printf(...) xil_printf(__VA_ARGS__)
#else
  #define info_printf(...)
#endif

#ifdef DEBUG_PRINT
  #define debug_printf(...) xil_printf(__VA_ARGS__)
#else
  #define debug_printf(...)
#endif

#ifdef TRACE_PRINT
  #define trace_printf(...) xil_printf(__VA_ARGS__)
#else
  #define trace_printf(...)
#endif

#endif
