#ifndef GK_MACRO_H
#define GK_MACRO_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Variable number space (FANUC style):
 *   #1..#33      local variables
 *   #100..#199   common variables
 *   #1000+       system variables
 */
#define GK_MACRO_LOCAL_BASE 1
#define GK_MACRO_LOCAL_COUNT 33
#define GK_MACRO_COMMON_BASE 100
#define GK_MACRO_COMMON_COUNT 100
#define GK_MACRO_SYSTEM_BASE 1000
#define GK_MACRO_SYSTEM_COUNT 64

typedef struct {
    double local[GK_MACRO_LOCAL_COUNT];
    double common[GK_MACRO_COMMON_COUNT];
    double system[GK_MACRO_SYSTEM_COUNT];
} gk_macro_vars;

void gk_macro_vars_init(gk_macro_vars *v);

int gk_macro_index_is_valid(int index);
/* Returns GK_OK and stores the value, or GK_ERR_INVALID_ARG. */
gk_status gk_macro_get(const gk_macro_vars *v, int index, double *out);
gk_status gk_macro_set(gk_macro_vars *v, int index, double value);

/* Evaluate a macro expression string, e.g. "#1+SIN[30]*ABS[-2]".
 * Supported: + - * / , parentheses []/(),
 * functions ABS/SQRT/SIN/COS/TAN/ASIN/ACOS/ATAN/ROUND/FIX/FUP/LN/EXP,
 * constants PI/E, and variable references #n.
 * Variable references still need a gk_macro_vars to resolve. */
gk_status gk_macro_eval(const gk_macro_vars *v, const char *expr, double *out);

/* Parse a macro assignment "var = expr" where var is "#n"; evaluates expr
 * and stores. Returns GK_OK on success. */
gk_status gk_macro_assign(gk_macro_vars *v, const char *stmt);

const char *gk_macro_func_name(int idx);
int gk_macro_func_count(void);

#ifdef __cplusplus
}
#endif

#endif
