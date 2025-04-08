#ifndef __ASSERTION_H__
#define __ASSERTION_H__

#include "define.h"

#define debug_break() __builtin_trap()

void report(const char *expr, const char *message, const char *file, int32_t line);

#define ar_assert(expr) { if(expr){} else {report(#expr, "", __FILE__, __LINE__); debug_break();} }
#define ar_assert_msg(expr, message) { if(expr){} else {report(#expr, message, __FILE__, __LINE__); debug_break();} }
#define ar_assert_debug(expr) { if(expr){} else {report(#expr, "", __FILE__, __LINE__); debug_break();} }

#endif //__ASSERTION_H__
