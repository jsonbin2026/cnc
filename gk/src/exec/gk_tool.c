#include "gk/gk_tool.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void gk_tool_table_init(gk_tool_table *t)
{
    if (t == NULL) {
        return;
    }
    memset(t, 0, sizeof(*t));
}

void gk_tool_table_free(gk_tool_table *t)
{
    if (t == NULL) {
        return;
    }
    free(t->tools);
    t->tools = NULL;
    t->count = 0;
    t->cap = 0;
}

gk_status gk_tool_table_add(gk_tool_table *t, const gk_tool *tool)
{
    gk_tool *mem;
    if (t == NULL || tool == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (tool->number < 1) {
        return GK_ERR_OUT_OF_RANGE;
    }
    if (gk_tool_table_get(t, tool->number) != NULL) {
        return GK_ERR_ALREADY_EXISTS;
    }
    if (t->count == t->cap) {
        size_t next = t->cap == 0 ? 16 : t->cap * 2;
        mem = realloc(t->tools, next * sizeof(*mem));
        if (mem == NULL) {
            return GK_ERR_NO_MEMORY;
        }
        t->tools = mem;
        t->cap = next;
    }
    t->tools[t->count] = *tool;
    t->count += 1;
    return GK_OK;
}

gk_tool *gk_tool_table_get(gk_tool_table *t, int number)
{
    size_t i;
    if (t == NULL) {
        return NULL;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->tools[i].number == number) {
            return &t->tools[i];
        }
    }
    return NULL;
}

size_t gk_tool_table_count(const gk_tool_table *t)
{
    return t != NULL ? t->count : 0;
}

gk_status gk_tool_table_remove(gk_tool_table *t, int number)
{
    size_t i;
    if (t == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    for (i = 0; i < t->count; ++i) {
        if (t->tools[i].number == number) {
            memmove(&t->tools[i], &t->tools[i + 1],
                    (t->count - i - 1) * sizeof(gk_tool));
            t->count -= 1;
            return GK_OK;
        }
    }
    return GK_ERR_NOT_FOUND;
}

gk_status gk_tool_consume_time(gk_tool_table *t, int number, double minutes)
{
    gk_tool *tool;
    if (t == NULL || minutes < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    tool = gk_tool_table_get(t, number);
    if (tool == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    tool->life_used += minutes;
    return GK_OK;
}

int gk_tool_needs_warning(const gk_tool *t)
{
    if (t == NULL || t->life_limit <= 0.0) {
        return 0;
    }
    if (t->life_warn <= 0.0) {
        return 0;
    }
    return t->life_used >= t->life_limit * t->life_warn;
}

int gk_tool_life_expired(const gk_tool *t)
{
    if (t == NULL || t->life_limit <= 0.0) {
        return 0;
    }
    return t->life_used >= t->life_limit;
}

gk_point3 gk_comp_apply(gk_cutter_comp mode, double radius,
                        gk_point3 prev, gk_point3 current,
                        gk_point3 next)
{
    gk_vec2 dir_out;
    gk_vec2 normal;
    double len;
    gk_point3 result = current;

    (void)prev;
    /* Direction of travel toward next (2D in XY). */
    dir_out.x = next.x - current.x;
    dir_out.y = next.y - current.y;
    len = sqrt(dir_out.x * dir_out.x + dir_out.y * dir_out.y);
    if (len <= GK_EPS) {
        return result;
    }
    dir_out.x /= len;
    dir_out.y /= len;

    /* Left normal in XY plane. */
    normal.x = -dir_out.y;
    normal.y = dir_out.x;
    if (mode == GK_COMP_RIGHT) {
        normal.x = -normal.x;
        normal.y = -normal.y;
    } else if (mode == GK_COMP_CANCEL) {
        return result;
    }

    result.x += normal.x * radius;
    result.y += normal.y * radius;
    return result;
}

double gk_len_comp_apply(gk_length_comp mode, double length, double z)
{
    switch (mode) {
    case GK_LEN_COMP_POS:
        return z - length;
    case GK_LEN_COMP_NEG:
        return z + length;
    case GK_LEN_COMP_CANCEL:
    default:
        return z;
    }
}
