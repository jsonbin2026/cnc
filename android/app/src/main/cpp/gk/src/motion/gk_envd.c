#include "gk/gk_envd.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void gk__envd_copy(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* ===================================================================
 * Workshop (1176)
 * =================================================================== */

void gk_envd_shop_init(gk_envd_shop *s, double length_m, double width_m,
                       double height_m)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->length_m = length_m;
    s->width_m = width_m;
    s->height_m = height_m;
    s->ambient_c = 25.0;
}

double gk_envd_shop_volume(const gk_envd_shop *s)
{
    if (s == NULL) {
        return 0.0;
    }
    return s->length_m * s->width_m * s->height_m;
}

double gk_envd_shop_floor_area(const gk_envd_shop *s)
{
    if (s == NULL) {
        return 0.0;
    }
    return s->length_m * s->width_m;
}

/* ===================================================================
 * Ambient sounds (1177-1187)
 * =================================================================== */

const char *gk_envd_sound_name(gk_envd_sound_kind k)
{
    switch (k) {
    case GK_ENVD_SND_MACHINE: return "machine";
    case GK_ENVD_SND_FORKLIFT: return "forklift";
    case GK_ENVD_SND_CRANE: return "crane";
    case GK_ENVD_SND_PEOPLE: return "people";
    case GK_ENVD_SND_BROADCAST: return "broadcast";
    case GK_ENVD_SND_VENTILATION: return "ventilation";
    case GK_ENVD_SND_AIRCON: return "aircon";
    case GK_ENVD_SND_TRANSFORMER: return "transformer";
    case GK_ENVD_SND_CABINET_FAN: return "cabinet-fan";
    case GK_ENVD_SND_COOLING_TOWER: return "cooling-tower";
    case GK_ENVD_SND_COMPRESSOR: return "compressor";
    default: return "unknown";
    }
}

double gk_envd_sound_base_db(gk_envd_sound_kind k)
{
    switch (k) {
    case GK_ENVD_SND_MACHINE: return 75.0;
    case GK_ENVD_SND_FORKLIFT: return 65.0;
    case GK_ENVD_SND_CRANE: return 60.0;
    case GK_ENVD_SND_PEOPLE: return 55.0;
    case GK_ENVD_SND_BROADCAST: return 58.0;
    case GK_ENVD_SND_VENTILATION: return 50.0;
    case GK_ENVD_SND_AIRCON: return 52.0;
    case GK_ENVD_SND_TRANSFORMER: return 48.0;
    case GK_ENVD_SND_CABINET_FAN: return 45.0;
    case GK_ENVD_SND_COOLING_TOWER: return 62.0;
    case GK_ENVD_SND_COMPRESSOR: return 70.0;
    default: return 0.0;
    }
}

void gk_envd_sound_init(gk_envd_sound *s, gk_envd_sound_kind kind,
                        double distance_m)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->kind = kind;
    s->distance_m = distance_m;
    s->reference_m = 1.0;
    s->base_db = gk_envd_sound_base_db(kind);
    s->enabled = 1;
}

double gk_envd_sound_level(const gk_envd_sound *s)
{
    double d;
    if (s == NULL || !s->enabled || s->distance_m <= 0.0) {
        return 0.0;
    }
    /* inverse-square attenuation in dB */
    d = s->distance_m / s->reference_m;
    if (d < 1.0) {
        d = 1.0;
    }
    return s->base_db - 20.0 * log10(d);
}

/* ===================================================================
 * Floor (1188-1190)
 * =================================================================== */

const char *gk_envd_floor_name(gk_envd_floor_state s)
{
    switch (s) {
    case GK_ENVD_FLOOR_CLEAN: return "clean";
    case GK_ENVD_FLOOR_OIL: return "oil";
    case GK_ENVD_FLOOR_CHIPS: return "chips";
    case GK_ENVD_FLOOR_WATER: return "water";
    default: return "unknown";
    }
}

void gk_envd_floor_init(gk_envd_floor *f)
{
    if (f == NULL) {
        return;
    }
    memset(f, 0, sizeof(*f));
    f->state = GK_ENVD_FLOOR_CLEAN;
    f->friction = 0.7;
}

gk_status gk_envd_floor_soil(gk_envd_floor *f, gk_envd_floor_state state,
                             double coverage)
{
    if (f == NULL || coverage < 0.0 || coverage > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    f->state = state;
    f->coverage = coverage;
    switch (state) {
    case GK_ENVD_FLOOR_OIL:
        f->friction = 0.7 - 0.4 * coverage;
        break;
    case GK_ENVD_FLOOR_WATER:
        f->friction = 0.7 - 0.5 * coverage;
        break;
    case GK_ENVD_FLOOR_CHIPS:
        f->friction = 0.7 - 0.1 * coverage;
        break;
    default:
        f->friction = 0.7;
        break;
    }
    return GK_OK;
}

int gk_envd_floor_slip_risk(const gk_envd_floor *f, double threshold)
{
    if (f == NULL) {
        return 0;
    }
    return f->friction < threshold;
}

/* ===================================================================
 * Wall sticker (1191)
 * =================================================================== */

gk_status gk_envd_wall_sticker(const char *text, char *out, size_t out_cap)
{
    int n;
    if (text == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "[SIGN] %s", text);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Aisle / fire equipment (1192-1193)
 * =================================================================== */

void gk_envd_aisle_init(gk_envd_aisle *a, double width_mm)
{
    if (a == NULL) {
        return;
    }
    memset(a, 0, sizeof(*a));
    a->width_mm = width_mm;
    a->marked = 1;
}

int gk_envd_aisle_ok(const gk_envd_aisle *a)
{
    if (a == NULL) {
        return 0;
    }
    return a->marked && !a->blocked && a->width_mm >= 800.0;
}

gk_status gk_envd_aisle_block(gk_envd_aisle *a, int blocked)
{
    if (a == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    a->blocked = blocked ? 1 : 0;
    return GK_OK;
}

void gk_envd_fire_init(gk_envd_fire_equipment *e, const char *kind, int count)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
    gk__envd_copy(e->kind, sizeof(e->kind), kind);
    e->count = count;
}

gk_status gk_envd_fire_inspect(gk_envd_fire_equipment *e, int passed)
{
    if (e == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    e->inspected = passed ? 1 : 0;
    return passed ? GK_OK : GK_ERR_STATE;
}

/* ===================================================================
 * Storage / waste (1194-1198)
 * =================================================================== */

const char *gk_envd_store_name(gk_envd_store_kind k)
{
    switch (k) {
    case GK_ENVD_STORE_TOOLBOX: return "toolbox";
    case GK_ENVD_STORE_MATERIAL: return "material-rack";
    case GK_ENVD_STORE_FINISHED: return "finished-rack";
    case GK_ENVD_STORE_WASTE: return "waste-bin";
    case GK_ENVD_STORE_CLEANING: return "cleaning-tools";
    default: return "unknown";
    }
}

void gk_envd_store_init(gk_envd_store *s, gk_envd_store_kind kind,
                        int capacity)
{
    if (s == NULL) {
        return;
    }
    memset(s, 0, sizeof(*s));
    s->kind = kind;
    s->capacity = capacity > 0 ? capacity : 1;
}

int gk_envd_store_count(const gk_envd_store *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->used;
}

gk_status gk_envd_store_put(gk_envd_store *s, int items)
{
    if (s == NULL || items < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (s->used + items > s->capacity) {
        return GK_ERR_OUT_OF_RANGE;
    }
    s->used += items;
    return GK_OK;
}

gk_status gk_envd_store_take(gk_envd_store *s, int items)
{
    if (s == NULL || items < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (items > s->used) {
        return GK_ERR_STATE;
    }
    s->used -= items;
    return GK_OK;
}

int gk_envd_store_full(const gk_envd_store *s)
{
    if (s == NULL) {
        return 0;
    }
    return s->used >= s->capacity;
}

/* ===================================================================
 * Lighting (1199-1200)
 * =================================================================== */

void gk_envd_lighting_init(gk_envd_lighting *l, int lamp_count,
                           double watts_each)
{
    if (l == NULL) {
        return;
    }
    memset(l, 0, sizeof(*l));
    l->lamp_count = lamp_count;
    l->watts_each = watts_each;
    l->level = 1.0;
}

double gk_envd_lighting_power(const gk_envd_lighting *l)
{
    if (l == NULL) {
        return 0.0;
    }
    return (double)(l->lamp_count - l->failed) * l->watts_each * l->level;
}

gk_status gk_envd_lighting_set_level(gk_envd_lighting *l, double level)
{
    if (l == NULL || level < 0.0 || level > 1.0) {
        return GK_ERR_OUT_OF_RANGE;
    }
    l->level = level;
    return GK_OK;
}

void gk_envd_emergency_light_init(gk_envd_emergency_light *e)
{
    if (e == NULL) {
        return;
    }
    memset(e, 0, sizeof(*e));
}

gk_status gk_envd_emergency_light_test(gk_envd_emergency_light *e,
                                       double duration_min)
{
    if (e == NULL || duration_min < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    e->on = 1;
    e->runtime_min = duration_min;
    return GK_OK;
}

int gk_envd_emergency_light_ok(const gk_envd_emergency_light *e)
{
    if (e == NULL) {
        return 0;
    }
    /* must sustain at least 90 minutes of backup */
    return e->on && e->runtime_min >= 90.0;
}

/* ===================================================================
 * Window / view (1201-1202)
 * =================================================================== */

void gk_envd_window_init(gk_envd_window *w, double width_mm, double height_mm)
{
    if (w == NULL) {
        return;
    }
    memset(w, 0, sizeof(*w));
    w->width_mm = width_mm;
    w->height_mm = height_mm;
    w->transparency = 0.9;
}

double gk_envd_window_area(const gk_envd_window *w)
{
    if (w == NULL) {
        return 0.0;
    }
    return w->width_mm * w->height_mm;
}

gk_status gk_envd_window_view(const char *outside, char *out, size_t out_cap)
{
    int n;
    if (outside == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "OUTSIDE:%s", outside);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Clock (1203)
 * =================================================================== */

void gk_envd_clock_init(gk_envd_clock *c)
{
    if (c == NULL) {
        return;
    }
    memset(c, 0, sizeof(*c));
}

gk_status gk_envd_clock_set(gk_envd_clock *c, int hour, int minute,
                            int second)
{
    if (c == NULL || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 59) {
        return GK_ERR_OUT_OF_RANGE;
    }
    c->hour = hour;
    c->minute = minute;
    c->second = second;
    return GK_OK;
}

gk_status gk_envd_clock_render(const gk_envd_clock *c, char *out,
                               size_t out_cap)
{
    int n;
    if (c == NULL || out == NULL || out_cap == 0) {
        return GK_ERR_INVALID_ARG;
    }
    n = snprintf(out, out_cap, "%02d:%02d:%02d", c->hour, c->minute,
                 c->second);
    if (n < 0 || (size_t)n >= out_cap) {
        return GK_ERR_OUT_OF_RANGE;
    }
    return GK_OK;
}

/* ===================================================================
 * Production board (1204)
 * =================================================================== */

void gk_envd_board_init(gk_envd_board *b, int target)
{
    if (b == NULL) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->target = target;
}

gk_status gk_envd_board_update(gk_envd_board *b, int planned, int completed)
{
    if (b == NULL || planned < 0 || completed < 0) {
        return GK_ERR_INVALID_ARG;
    }
    b->planned = planned;
    b->completed = completed;
    return GK_OK;
}

double gk_envd_board_achievement(const gk_envd_board *b)
{
    if (b == NULL || b->target <= 0) {
        return 0.0;
    }
    return (double)b->completed / (double)b->target;
}

/* ===================================================================
 * Shift handover (1205)
 * =================================================================== */

void gk_envd_handover_init(gk_envd_handover *h)
{
    if (h == NULL) {
        return;
    }
    memset(h, 0, sizeof(*h));
}

gk_status gk_envd_handover_set(gk_envd_handover *h, const char *outgoing,
                               const char *incoming, const char *note)
{
    if (h == NULL || outgoing == NULL || incoming == NULL || note == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    gk__envd_copy(h->outgoing, sizeof(h->outgoing), outgoing);
    gk__envd_copy(h->incoming, sizeof(h->incoming), incoming);
    gk__envd_copy(h->note, sizeof(h->note), note);
    return GK_OK;
}

gk_status gk_envd_handover_sign(gk_envd_handover *h)
{
    if (h == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (h->outgoing[0] == '\0' || h->incoming[0] == '\0') {
        return GK_ERR_STATE;
    }
    h->signed_off = 1;
    return GK_OK;
}
