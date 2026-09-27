#ifndef GK_CODES_H
#define GK_CODES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int code;
    int sub;          /* decimal sub-code, e.g. G06.2 -> sub=2 */
    const char *name;
} gk_code_def;

const gk_code_def *gk_gcode_table(size_t *count);
const gk_code_def *gk_mcode_table(size_t *count);
const char *gk_gcode_lookup(int code, int sub);
const char *gk_mcode_lookup(int code);

#ifdef __cplusplus
}
#endif

#endif
