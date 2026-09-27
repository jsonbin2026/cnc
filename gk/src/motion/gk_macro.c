#include "gk/gk_macro.h"

#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void gk_macro_vars_init(gk_macro_vars *v)
{
    if (v == NULL) {
        return;
    }
    memset(v, 0, sizeof(*v));
}

int gk_macro_index_is_valid(int index)
{
    if (index >= GK_MACRO_LOCAL_BASE &&
        index < GK_MACRO_LOCAL_BASE + GK_MACRO_LOCAL_COUNT) {
        return 1;
    }
    if (index >= GK_MACRO_COMMON_BASE &&
        index < GK_MACRO_COMMON_BASE + GK_MACRO_COMMON_COUNT) {
        return 1;
    }
    if (index >= GK_MACRO_SYSTEM_BASE &&
        index < GK_MACRO_SYSTEM_BASE + GK_MACRO_SYSTEM_COUNT) {
        return 1;
    }
    return 0;
}

static double *macro_slot(gk_macro_vars *v, int index)
{
    if (index >= GK_MACRO_LOCAL_BASE &&
        index < GK_MACRO_LOCAL_BASE + GK_MACRO_LOCAL_COUNT) {
        return &v->local[index - GK_MACRO_LOCAL_BASE];
    }
    if (index >= GK_MACRO_COMMON_BASE &&
        index < GK_MACRO_COMMON_BASE + GK_MACRO_COMMON_COUNT) {
        return &v->common[index - GK_MACRO_COMMON_BASE];
    }
    if (index >= GK_MACRO_SYSTEM_BASE &&
        index < GK_MACRO_SYSTEM_BASE + GK_MACRO_SYSTEM_COUNT) {
        return &v->system[index - GK_MACRO_SYSTEM_BASE];
    }
    return NULL;
}

gk_status gk_macro_get(const gk_macro_vars *v, int index, double *out)
{
    double *slot;
    if (v == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    slot = macro_slot((gk_macro_vars *)v, index);
    if (slot == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    *out = *slot;
    return GK_OK;
}

gk_status gk_macro_set(gk_macro_vars *v, int index, double value)
{
    double *slot;
    if (v == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    slot = macro_slot(v, index);
    if (slot == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    *slot = value;
    return GK_OK;
}

/* ---- expression evaluator ---- */

typedef struct {
    const char *s;
    size_t pos;
    const gk_macro_vars *vars;
    gk_status error;
} macro_parser;

static void mp_skip_ws(macro_parser *mp)
{
    while (mp->s[mp->pos] == ' ' || mp->s[mp->pos] == '\t') {
        mp->pos += 1;
    }
}

static int mp_match(macro_parser *mp, char c)
{
    mp_skip_ws(mp);
    if (mp->s[mp->pos] == c) {
        mp->pos += 1;
        return 1;
    }
    return 0;
}

static double mp_expr(macro_parser *mp);

static double mp_primary(macro_parser *mp)
{
    mp_skip_ws(mp);
    if (mp->error != GK_OK) {
        return 0.0;
    }
    if (mp_match(mp, '(')) {
        double val = mp_expr(mp);
        if (!mp_match(mp, ')')) {
            mp->error = GK_ERR_PARSE;
            return 0.0;
        }
        return val;
    }
    if (mp_match(mp, '[')) {
        double val = mp_expr(mp);
        if (!mp_match(mp, ']')) {
            mp->error = GK_ERR_PARSE;
            return 0.0;
        }
        return val;
    }
    if (mp_match(mp, '+')) {
        return mp_primary(mp);
    }
    if (mp_match(mp, '-')) {
        return -mp_primary(mp);
    }
    if (mp->s[mp->pos] == '#') {
        int index = 0;
        double value;
        mp->pos += 1;
        if (!isdigit((unsigned char)mp->s[mp->pos])) {
            /* indirect variable # [expr] */
            if (mp_match(mp, '[')) {
                double iv = mp_expr(mp);
                if (!mp_match(mp, ']')) {
                    mp->error = GK_ERR_PARSE;
                    return 0.0;
                }
                index = (int)floor(iv + 0.5);
            } else {
                mp->error = GK_ERR_PARSE;
                return 0.0;
            }
        } else {
            while (isdigit((unsigned char)mp->s[mp->pos])) {
                index = index * 10 + (mp->s[mp->pos] - '0');
                mp->pos += 1;
            }
        }
        if (gk_macro_get(mp->vars, index, &value) != GK_OK) {
            mp->error = GK_ERR_OUT_OF_RANGE;
            return 0.0;
        }
        return value;
    }
    if (isalpha((unsigned char)mp->s[mp->pos]) ||
        mp->s[mp->pos] == '_') {
        char name[16];
        size_t n = 0;
        while ((isalnum((unsigned char)mp->s[mp->pos]) ||
                mp->s[mp->pos] == '_') &&
               n < sizeof(name) - 1) {
            name[n++] = (char)toupper((unsigned char)mp->s[mp->pos]);
            mp->pos += 1;
        }
        name[n] = '\0';
        if (strcmp(name, "PI") == 0) {
            return 3.14159265358979323846;
        }
        if (strcmp(name, "E") == 0) {
            return 2.71828182845904523536;
        }
        /* function call */
        mp_skip_ws(mp);
        if (!mp_match(mp, '[')) {
            mp->error = GK_ERR_PARSE;
            return 0.0;
        }
        {
            double a = mp_expr(mp);
            if (!mp_match(mp, ']')) {
                mp->error = GK_ERR_PARSE;
                return 0.0;
            }
            if (strcmp(name, "ABS") == 0) {
                return fabs(a);
            }
            if (strcmp(name, "SQRT") == 0) {
                if (a < 0.0) {
                    mp->error = GK_ERR_OUT_OF_RANGE;
                    return 0.0;
                }
                return sqrt(a);
            }
            if (strcmp(name, "SIN") == 0) {
                return sin(a * 3.14159265358979323846 / 180.0);
            }
            if (strcmp(name, "COS") == 0) {
                return cos(a * 3.14159265358979323846 / 180.0);
            }
            if (strcmp(name, "TAN") == 0) {
                return tan(a * 3.14159265358979323846 / 180.0);
            }
            if (strcmp(name, "ASIN") == 0) {
                return asin(a) * 180.0 / 3.14159265358979323846;
            }
            if (strcmp(name, "ACOS") == 0) {
                return acos(a) * 180.0 / 3.14159265358979323846;
            }
            if (strcmp(name, "ATAN") == 0) {
                return atan(a) * 180.0 / 3.14159265358979323846;
            }
            if (strcmp(name, "ROUND") == 0) {
                return floor(a + 0.5);
            }
            if (strcmp(name, "FIX") == 0) {
                return floor(a);
            }
            if (strcmp(name, "FUP") == 0) {
                return ceil(a);
            }
            if (strcmp(name, "LN") == 0) {
                if (a <= 0.0) {
                    mp->error = GK_ERR_OUT_OF_RANGE;
                    return 0.0;
                }
                return log(a);
            }
            if (strcmp(name, "EXP") == 0) {
                return exp(a);
            }
            mp->error = GK_ERR_NOT_FOUND;
            return 0.0;
        }
    }
    if (isdigit((unsigned char)mp->s[mp->pos]) ||
        mp->s[mp->pos] == '.') {
        char *end = NULL;
        double val = strtod(mp->s + mp->pos, &end);
        mp->pos = (size_t)(end - mp->s);
        return val;
    }
    mp->error = GK_ERR_PARSE;
    return 0.0;
}

static double mp_term(macro_parser *mp)
{
    double left = mp_primary(mp);
    for (;;) {
        mp_skip_ws(mp);
        if (mp->s[mp->pos] == '*') {
            mp->pos += 1;
            left *= mp_primary(mp);
        } else if (mp->s[mp->pos] == '/') {
            double rhs;
            mp->pos += 1;
            rhs = mp_primary(mp);
            if (rhs == 0.0) {
                mp->error = GK_ERR_OUT_OF_RANGE;
                return 0.0;
            }
            left /= rhs;
        } else if (strncmp(mp->s + mp->pos, "MOD", 3) == 0 &&
                   !isalnum((unsigned char)mp->s[mp->pos + 3])) {
            double rhs;
            mp->pos += 3;
            rhs = mp_primary(mp);
            if (rhs == 0.0) {
                mp->error = GK_ERR_OUT_OF_RANGE;
                return 0.0;
            }
            left = fmod(left, rhs);
        } else {
            break;
        }
    }
    return left;
}

static double mp_expr(macro_parser *mp)
{
    double left = mp_term(mp);
    for (;;) {
        mp_skip_ws(mp);
        if (mp->s[mp->pos] == '+') {
            mp->pos += 1;
            left += mp_term(mp);
        } else if (mp->s[mp->pos] == '-') {
            mp->pos += 1;
            left -= mp_term(mp);
        } else {
            break;
        }
    }
    return left;
}

gk_status gk_macro_eval(const gk_macro_vars *v, const char *expr, double *out)
{
    macro_parser mp;
    double val;
    if (expr == NULL || out == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    mp.s = expr;
    mp.pos = 0;
    mp.vars = v;
    mp.error = GK_OK;
    val = mp_expr(&mp);
    mp_skip_ws(&mp);
    if (mp.error != GK_OK) {
        return mp.error;
    }
    if (mp.s[mp.pos] != '\0') {
        return GK_ERR_PARSE;
    }
    *out = val;
    return GK_OK;
}

gk_status gk_macro_assign(gk_macro_vars *v, const char *stmt)
{
    const char *p;
    int index = 0;
    double value;
    gk_status st;
    if (v == NULL || stmt == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    p = stmt;
    while (*p == ' ' || *p == '\t') {
        p += 1;
    }
    if (*p != '#') {
        return GK_ERR_PARSE;
    }
    p += 1;
    if (!isdigit((unsigned char)*p)) {
        return GK_ERR_PARSE;
    }
    while (isdigit((unsigned char)*p)) {
        index = index * 10 + (*p - '0');
        p += 1;
    }
    while (*p == ' ' || *p == '\t') {
        p += 1;
    }
    if (*p != '=') {
        return GK_ERR_PARSE;
    }
    p += 1;
    st = gk_macro_eval(v, p, &value);
    if (st != GK_OK) {
        return st;
    }
    return gk_macro_set(v, index, value);
}

static const char *const g_macro_funcs[] = {
    "ABS", "SQRT", "SIN", "COS", "TAN", "ASIN", "ACOS", "ATAN",
    "ROUND", "FIX", "FUP", "LN", "EXP"
};

const char *gk_macro_func_name(int idx)
{
    if (idx < 0 || idx >= (int)(sizeof(g_macro_funcs) / sizeof(g_macro_funcs[0]))) {
        return NULL;
    }
    return g_macro_funcs[idx];
}

int gk_macro_func_count(void)
{
    return (int)(sizeof(g_macro_funcs) / sizeof(g_macro_funcs[0]));
}
