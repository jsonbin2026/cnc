#ifndef GK_TOOL_H
#define GK_TOOL_H

#include <stddef.h>
#include "gk/gk_error.h"
#include "gk/gk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_COMP_CANCEL = 0,   /* G40 */
    GK_COMP_LEFT,         /* G41 */
    GK_COMP_RIGHT         /* G42 */
} gk_cutter_comp;

typedef enum {
    GK_LEN_COMP_CANCEL = 0, /* G49 */
    GK_LEN_COMP_POS,        /* G43 */
    GK_LEN_COMP_NEG         /* G44 */
} gk_length_comp;

typedef struct {
    int number;             /* T number */
    double diameter;        /* mm */
    double length;          /* mm */
    double wear_radius;     /* accumulated radial wear, mm */
    double wear_length;     /* accumulated length wear, mm */
    double life_limit;      /* minutes, 0 = unlimited */
    double life_used;       /* minutes consumed */
    double life_warn;       /* warning threshold fraction 0..1 */
    int    broken;
    int    in_use;
    char   name[32];
} gk_tool;

typedef struct {
    gk_tool *tools;
    size_t count;
    size_t cap;
} gk_tool_table;

void gk_tool_table_init(gk_tool_table *t);
void gk_tool_table_free(gk_tool_table *t);
gk_status gk_tool_table_add(gk_tool_table *t, const gk_tool *tool);
gk_tool *gk_tool_table_get(gk_tool_table *t, int number);
size_t gk_tool_table_count(const gk_tool_table *t);
gk_status gk_tool_table_remove(gk_tool_table *t, int number);

gk_status gk_tool_consume_time(gk_tool_table *t, int number, double minutes);
int gk_tool_needs_warning(const gk_tool *t);
int gk_tool_life_expired(const gk_tool *t);

/* Cutter compensation: offset a 2D path point perpendicular to motion. */
gk_point3 gk_comp_apply(gk_cutter_comp mode, double radius,
                        gk_point3 prev, gk_point3 current,
                        gk_point3 next);
/* Apply length compensation to Z. */
double gk_len_comp_apply(gk_length_comp mode, double length, double z);

#ifdef __cplusplus
}
#endif

#endif
